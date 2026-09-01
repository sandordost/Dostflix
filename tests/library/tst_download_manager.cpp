#include "app/AppSettings.h"
#include "library/DownloadManager.h"
#include "library/LibraryDatabase.h"
#include "library/LibraryManager.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QSignalSpy>
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
};

QTEST_GUILESS_MAIN(DownloadManagerTest)
#include "tst_download_manager.moc"
