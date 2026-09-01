#include "app/AppSettings.h"
#include "library/DownloadManager.h"
#include "library/LibraryDatabase.h"
#include "library/LibraryManager.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QSignalSpy>
#include <QStorageInfo>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

class RangeServer final : public QObject
{
public:
    explicit RangeServer(QByteArray data, QObject *parent = nullptr)
        : QObject(parent), payload(std::move(data))
    {
        connect(&server, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *socket = server.nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
                    buffers[socket] += socket->readAll();
                    if (!buffers[socket].contains("\r\n\r\n")) return;
                    const QByteArray request = buffers.take(socket);
                    requests.append(request);
                    qint64 start = 0;
                    for (const QByteArray &line : request.split('\n')) {
                        if (line.toLower().startsWith("range: bytes=")) {
                            start = line.mid(line.indexOf('=') + 1)
                                        .split('-').first().trimmed().toLongLong();
                        }
                    }
                    const QByteArray body = payload.mid(start);
                    QByteArray response;
                    if (start > 0) {
                        response = "HTTP/1.1 206 Partial Content\r\nContent-Range: bytes "
                            + QByteArray::number(start) + '-' + QByteArray::number(payload.size() - 1)
                            + '/' + QByteArray::number(payload.size()) + "\r\n";
                    } else {
                        response = "HTTP/1.1 200 OK\r\n";
                    }
                    response += "Accept-Ranges: bytes\r\nContent-Length: "
                        + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
                    socket->write(response);
                    socket->disconnectFromHost();
                });
            }
        });
    }

    bool listen() { return server.listen(QHostAddress::LocalHost); }
    QUrl url() const
    { return QUrl(QStringLiteral("http://127.0.0.1:%1/movie").arg(server.serverPort())); }

    QByteArray payload;
    QList<QByteArray> requests;

private:
    QHash<QTcpSocket *, QByteArray> buffers;
    QTcpServer server;
};

class DownloadManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void savesCompleteMovieAtomically()
    {
        QTemporaryDir root;
        const QByteArray payload(128 * 1024, 'D');
        RangeServer server(payload);
        QVERIFY(server.listen());
        AppSettings settings(root.filePath(QStringLiteral("settings.ini")));
        LibraryDatabase database(root.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("download-complete-test"));
        QVERIFY(database.open());
        const QString libraryPath = root.filePath(QStringLiteral("Movies"));
        LibraryManager library(settings, database, libraryPath);
        DownloadManager download(database, library);
        download.setNetworkReady(true);

        download.beginTransfer(QStringLiteral("Movie"), QStringLiteral("hash-one"), 1,
                               QStringLiteral("folder/movie.mkv"), payload.size(), server.url());
        QTRY_VERIFY_WITH_TIMEOUT(!download.hasPending(), 3'000);
        QFile completed(QDir(libraryPath).filePath(QStringLiteral("movie.mkv")));
        QVERIFY(completed.open(QIODevice::ReadOnly));
        QCOMPARE(completed.readAll(), payload);
        QVERIFY(!QFileInfo::exists(completed.fileName() + QStringLiteral(".dostflix.part")));
        QCOMPARE(library.count(), 1);
        QCOMPARE(database.transfer(QStringLiteral("hash-one"), 1)->state,
                 QStringLiteral("completed"));
    }

    void resumesFromPersistedPartialFile()
    {
        QTemporaryDir root;
        const QByteArray payload("0123456789abcdefghijklmnopqrstuvwxyz");
        RangeServer server(payload);
        QVERIFY(server.listen());
        AppSettings settings(root.filePath(QStringLiteral("settings.ini")));
        LibraryDatabase database(root.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("download-resume-test"));
        QVERIFY(database.open());
        const QString libraryPath = root.filePath(QStringLiteral("Movies"));
        QDir().mkpath(libraryPath);
        const QString finalPath = QDir(libraryPath).filePath(QStringLiteral("resume.mp4"));
        const QString partialPath = finalPath + QStringLiteral(".dostflix.part");
        QFile partial(partialPath);
        QVERIFY(partial.open(QIODevice::WriteOnly));
        QCOMPARE(partial.write(payload.first(10)), 10);
        partial.close();
        QVERIFY(database.saveTransfer({QStringLiteral("hash-two"), 2,
            QStringLiteral("Resume movie"), QStringLiteral("resume.mp4"), payload.size(),
            partialPath, finalPath, 10, QStringLiteral("paused")}));

        LibraryManager library(settings, database, libraryPath);
        DownloadManager download(database, library);
        QSignalSpy resumeSpy(&download, &DownloadManager::resumeRequested);
        download.setNetworkReady(true);
        QCOMPARE(resumeSpy.size(), 1);
        download.beginTransfer(QStringLiteral("Resume movie"), QStringLiteral("hash-two"), 2,
                               QStringLiteral("resume.mp4"), payload.size(), server.url());
        QTRY_VERIFY_WITH_TIMEOUT(!download.hasPending(), 3'000);
        QVERIFY(server.requests.first().contains("Range: bytes=10-"));
        QFile completed(finalPath);
        QVERIFY(completed.open(QIODevice::ReadOnly));
        QCOMPARE(completed.readAll(), payload);
    }

    void finalizesCompletePartialWithoutNetwork()
    {
        QTemporaryDir root;
        const QByteArray payload("already complete");
        AppSettings settings(root.filePath(QStringLiteral("settings.ini")));
        LibraryDatabase database(root.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("download-offline-finalize-test"));
        QVERIFY(database.open());
        const QString libraryPath = root.filePath(QStringLiteral("Movies"));
        QDir().mkpath(libraryPath);
        const QString finalPath = QDir(libraryPath).filePath(QStringLiteral("offline.mkv"));
        const QString partialPath = finalPath + QStringLiteral(".dostflix.part");
        QFile partial(partialPath);
        QVERIFY(partial.open(QIODevice::WriteOnly));
        QCOMPARE(partial.write(payload), payload.size());
        partial.close();
        QVERIFY(database.saveTransfer({QStringLiteral("hash-three"), 4,
            QStringLiteral("Offline finalization"), QStringLiteral("offline.mkv"), payload.size(),
            partialPath, finalPath, payload.size(), QStringLiteral("downloading")}));

        LibraryManager library(settings, database, libraryPath);
        DownloadManager download(database, library);
        QVERIFY(!download.hasPending());
        QVERIFY(QFileInfo::exists(finalPath));
        QVERIFY(!QFileInfo::exists(partialPath));
        QCOMPARE(library.count(), 1);
        QCOMPARE(database.transfer(QStringLiteral("hash-three"), 4)->state,
                 QStringLiteral("completed"));
    }

    void blocksTransferBeforeDiskCanFill()
    {
        QTemporaryDir root;
        RangeServer server(QByteArray("unused"));
        QVERIFY(server.listen());
        AppSettings settings(root.filePath(QStringLiteral("settings.ini")));
        LibraryDatabase database(root.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("download-disk-space-test"));
        QVERIFY(database.open());
        const QString libraryPath = root.filePath(QStringLiteral("Movies"));
        LibraryManager library(settings, database, libraryPath);
        const qint64 available = QStorageInfo(libraryPath).bytesAvailable();
        QVERIFY(available > 0);
        DownloadManager download(database, library);
        download.setNetworkReady(true);

        download.beginTransfer(QStringLiteral("Too large"), QStringLiteral("hash-large"), 0,
                               QStringLiteral("large.mkv"), available + 1, server.url());

        QVERIFY(download.hasPending());
        QVERIFY(!download.active());
        QVERIFY(!download.diskSpaceReady());
        QVERIFY(download.errorMessage().contains(QStringLiteral("Not enough free space")));
        QCOMPARE(download.stateLabel(), QStringLiteral("Waiting for disk space"));
        QCOMPARE(server.requests.size(), 0);
        const auto stored = database.transfer(QStringLiteral("hash-large"), 0);
        QVERIFY(stored.has_value());
        QCOMPARE(stored->state, QStringLiteral("paused"));
        QVERIFY(!QFileInfo::exists(stored->partialPath));
    }

    void playsCompletedDownloadLocally()
    {
        QTemporaryDir root;
        const QByteArray payload("complete movie");
        AppSettings settings(root.filePath(QStringLiteral("settings.ini")));
        LibraryDatabase database(root.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("download-play-complete-test"));
        QVERIFY(database.open());
        const QString libraryPath = root.filePath(QStringLiteral("Movies"));
        QDir().mkpath(libraryPath);
        const QString finalPath = QDir(libraryPath).filePath(QStringLiteral("complete.mkv"));
        QFile completed(finalPath);
        QVERIFY(completed.open(QIODevice::WriteOnly));
        QCOMPARE(completed.write(payload), payload.size());
        completed.close();
        QVERIFY(database.saveTransfer({QString(40, QLatin1Char('a')), 0,
            QStringLiteral("Complete movie"), QStringLiteral("complete.mkv"), payload.size(),
            finalPath + QStringLiteral(".dostflix.part"), finalPath, payload.size(),
            QStringLiteral("completed")}));

        LibraryManager library(settings, database, libraryPath);
        DownloadManager download(database, library);
        QSignalSpy playbackSpy(&download, &DownloadManager::localPlaybackRequested);
        QVERIFY(download.hasTransfer());
        QVERIFY(download.playable());
        download.play();
        QCOMPARE(playbackSpy.size(), 1);
        QCOMPARE(playbackSpy.first().first().toUrl(), QUrl::fromLocalFile(finalPath));
    }

    void reusesMatchingMagnetAndExistingPartial()
    {
        QTemporaryDir root;
        AppSettings settings(root.filePath(QStringLiteral("settings.ini")));
        LibraryDatabase database(root.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("download-reuse-test"));
        QVERIFY(database.open());
        const QString libraryPath = root.filePath(QStringLiteral("Movies"));
        QDir().mkpath(libraryPath);
        const QString finalPath = QDir(libraryPath).filePath(QStringLiteral("partial.mkv"));
        QFile partial(finalPath + QStringLiteral(".dostflix.part"));
        QVERIFY(partial.open(QIODevice::WriteOnly));
        QCOMPARE(partial.write("buffered"), 8);
        partial.close();
        const QString hash(40, QLatin1Char('a'));
        QVERIFY(database.saveTransfer({hash, 7, QStringLiteral("Partial movie"),
            QStringLiteral("partial.mkv"), 100, partial.fileName(), finalPath, 8,
            QStringLiteral("paused")}));

        LibraryManager library(settings, database, libraryPath);
        DownloadManager download(database, library);
        download.setNetworkReady(true);
        QSignalSpy playbackSpy(&download, &DownloadManager::torrentPlaybackRequested);
        QVERIFY(download.playMatchingRelease(
            QStringLiteral("Selected title"),
            QStringLiteral("magnet:?xt=urn:btih:") + hash.toUpper()));
        QCOMPARE(playbackSpy.size(), 1);
        QCOMPARE(playbackSpy.first().at(1).toString(), hash);
        QCOMPARE(playbackSpy.first().at(2).toInt(), 7);
        QCOMPARE(download.bytesWritten(), 8);
    }

    void removesFilesAndTransferHistory()
    {
        QTemporaryDir root;
        AppSettings settings(root.filePath(QStringLiteral("settings.ini")));
        LibraryDatabase database(root.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("download-remove-test"));
        QVERIFY(database.open());
        const QString libraryPath = root.filePath(QStringLiteral("Movies"));
        QDir().mkpath(libraryPath);
        const QString finalPath = QDir(libraryPath).filePath(QStringLiteral("remove.mkv"));
        QFile file(finalPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("remove me"), 9);
        file.close();
        QVERIFY(database.upsertMovie(QStringLiteral("Remove"), finalPath));
        QVERIFY(database.saveTransfer({QStringLiteral("remove-hash"), 2,
            QStringLiteral("Remove"), QStringLiteral("remove.mkv"), 9,
            finalPath + QStringLiteral(".dostflix.part"), finalPath, 9,
            QStringLiteral("completed")}));

        LibraryManager library(settings, database, libraryPath);
        DownloadManager download(database, library);
        QSignalSpy removalSpy(&download, &DownloadManager::torrentRemovalRequested);
        download.remove();
        QVERIFY(!QFileInfo::exists(finalPath));
        QVERIFY(!database.transfer(QStringLiteral("remove-hash"), 2).has_value());
        QCOMPARE(database.movies().size(), 0);
        QVERIFY(!download.hasTransfer());
        QCOMPARE(removalSpy.size(), 1);
    }
};

QTEST_GUILESS_MAIN(DownloadManagerTest)
#include "tst_download_manager.moc"
