#include "providers/SecretStore.h"
#include "subtitles/OpenSubtitlesManager.h"
#include "app/AppSettings.h"

#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

class MemorySecretStore final : public SecretStore
{
public:
    bool store(const QString &id, const QString &secret, QString *) override
    { values[id] = secret; return true; }
    QString load(const QString &id, QString *) override { return values.value(id); }
    bool remove(const QString &id, QString *) override { return values.remove(id) > 0; }
    QHash<QString, QString> values;
};

class FakeOpenSubtitlesServer final : public QObject
{
public:
    explicit FakeOpenSubtitlesServer(QObject *parent = nullptr) : QObject(parent)
    {
        connect(&server, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *socket = server.nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
                    buffers[socket] += socket->readAll();
                    QByteArray &request = buffers[socket];
                    const qsizetype headerEnd = request.indexOf("\r\n\r\n");
                    if (headerEnd < 0) return;
                    qsizetype contentLength = 0;
                    for (const QByteArray &line : request.left(headerEnd).split('\n')) {
                        if (line.toLower().startsWith("content-length:"))
                            contentLength = line.mid(line.indexOf(':') + 1).trimmed().toLongLong();
                    }
                    if (request.size() < headerEnd + 4 + contentLength) return;
                    requests.append(request);
                    respond(socket, request.left(request.indexOf("\r\n")));
                    buffers.remove(socket);
                });
                connect(socket, &QObject::destroyed, this,
                        [this, socket] { buffers.remove(socket); });
            }
        });
    }

    bool listen() { return server.listen(QHostAddress::AnyIPv4); }
    QUrl apiBase() const
    {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/api/v1/").arg(server.serverPort()));
    }

    QList<QByteArray> requests;

private:
    void respond(QTcpSocket *socket, const QByteArray &requestLine)
    {
        QByteArray body;
        QByteArray contentType = "application/json";
        if (requestLine.startsWith("POST /api/v1/login ")) {
            body = R"({"token":"test-token","base_url":"127.0.0.2"})";
        } else if (requestLine.startsWith("GET /api/v1/subtitles?")) {
            body = R"({"data":[{"attributes":{"language":"nl","release":"Test.Release.1080p","download_count":123,"hearing_impaired":false,"from_trusted":true,"files":[{"file_id":42,"file_name":"Test.Release.nl.srt"}]}}]})";
        } else if (requestLine.startsWith("POST /api/v1/download ")) {
            body = QStringLiteral("{\"link\":\"http://127.0.0.1:%1/file.srt\",\"file_name\":\"Test.Release.nl.srt\"}")
                       .arg(server.serverPort()).toUtf8();
        } else if (requestLine.startsWith("GET /file.srt ")) {
            contentType = "application/x-subrip";
            body = "1\n00:00:01,000 --> 00:00:02,000\nHallo\n";
        } else {
            body = R"({"message":"not found"})";
        }
        const QByteArray response = "HTTP/1.1 200 OK\r\nContent-Type: " + contentType
            + "\r\nContent-Length: " + QByteArray::number(body.size())
            + "\r\nConnection: close\r\n\r\n" + body;
        socket->write(response);
        socket->disconnectFromHost();
    }

    QHash<QTcpSocket *, QByteArray> buffers;
    QTcpServer server;
};

class OpenSubtitlesManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void requiresVpnBeforeNetworkAccess()
    {
        MemorySecretStore secrets;
        QTemporaryDir data;
        AppSettings settings(data.filePath(QStringLiteral("settings.ini")));
        FakeOpenSubtitlesServer server;
        QVERIFY(server.listen());
        OpenSubtitlesManager manager(settings, secrets, data.path(), server.apiBase());
        QVERIFY(manager.saveCredentials("api-key", "viewer", "secret"));

        manager.search("Test movie");
        QCOMPARE(server.requests.size(), 0);
        QVERIFY(manager.errorMessage().contains("VPN"));
    }

    void searchesDownloadsAndSavesSubtitle()
    {
        MemorySecretStore secrets;
        QTemporaryDir data;
        AppSettings settings(data.filePath(QStringLiteral("settings.ini")));
        FakeOpenSubtitlesServer server;
        QVERIFY(server.listen());
        OpenSubtitlesManager manager(settings, secrets, data.path(), server.apiBase());
        QVERIFY(manager.saveCredentials("api-key", "viewer", "secret"));
        QVERIFY(secrets.values.value("subtitles-opensubtitles").contains("api-key"));
        QVERIFY(manager.setPreferredLanguages("DE, nl, de"));
        QCOMPARE(manager.preferredLanguages(), QString("de,nl"));
        QCOMPARE(settings.subtitleLanguages(), QString("de,nl"));
        QVERIFY(!manager.setPreferredLanguages("not-a-language"));
        QCOMPARE(manager.preferredLanguages(), QString("de,nl"));

        const QString videoPath = data.filePath(QStringLiteral("Test Movie.mkv"));
        QFile video(videoPath);
        QVERIFY(video.open(QIODevice::WriteOnly));
        QCOMPARE(video.write(QByteArray(131072, '\0')), 131072);
        video.close();
        manager.setMediaContext(QUrl::fromLocalFile(videoPath), QStringLiteral("tt1234567"));
        manager.setNetworkReady(true);

        manager.search("Test movie");
        QTRY_COMPARE_WITH_TIMEOUT(manager.results().size(), 1, 3000);
        QCOMPARE(manager.results().first().toMap().value("fileId").toInt(), 42);
        QCOMPARE(manager.results().first().toMap().value("language").toString(), QString("nl"));
        QVERIFY(server.requests.at(0).contains("Api-Key: api-key"));
        QVERIFY(server.requests.at(0).contains("Accept: application/json"));
        QVERIFY(server.requests.at(1).contains("languages=de,nl"));
        QVERIFY(server.requests.at(1).contains("imdb_id=1234567"));
        QVERIFY(server.requests.at(1).contains("moviehash=0000000000020000"));
        QVERIFY(server.requests.at(1).contains("moviebytesize=131072"));
        QVERIFY(server.requests.at(1).contains("Host: 127.0.0.2:"));

        QSignalSpy readySpy(&manager, &OpenSubtitlesManager::subtitleReady);
        manager.download(0);
        QTRY_COMPARE_WITH_TIMEOUT(readySpy.size(), 1, 3000);
        QCOMPARE(server.requests.size(), 4);
        QVERIFY(server.requests.at(2).contains("Authorization: Bearer test-token"));
        QVERIFY(server.requests.at(2).contains("Accept: application/json"));
        const QUrl result = readySpy.first().first().toUrl();
        QCOMPARE(result.toLocalFile(), data.path() + "/Test Movie.nl.srt");
        QFile file(result.toLocalFile());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll().contains("Hallo"));
    }

    void savesBesideFutureDurableVideo()
    {
        MemorySecretStore secrets;
        QTemporaryDir data;
        AppSettings settings(data.filePath(QStringLiteral("settings.ini")));
        FakeOpenSubtitlesServer server;
        QVERIFY(server.listen());
        OpenSubtitlesManager manager(settings, secrets, data.path(), server.apiBase());
        QVERIFY(manager.saveCredentials("api-key", "viewer", "secret"));
        const QString videoPath = data.filePath(QStringLiteral("library/Future Movie.mkv"));
        manager.setMediaContext(QUrl::fromLocalFile(videoPath));
        manager.setNetworkReady(true);
        manager.search("Future Movie");
        QTRY_COMPARE_WITH_TIMEOUT(manager.results().size(), 1, 3000);
        QSignalSpy readySpy(&manager, &OpenSubtitlesManager::subtitleReady);
        manager.download(0);
        QTRY_COMPARE_WITH_TIMEOUT(readySpy.size(), 1, 3000);
        const QString savedPath = readySpy.first().first().toUrl().toLocalFile();
        QCOMPARE(savedPath, data.filePath(QStringLiteral("library/Future Movie.nl.srt")));
        QVERIFY(QFileInfo::exists(savedPath));
        QVERIFY(!QFileInfo::exists(videoPath));
    }
};

QTEST_MAIN(OpenSubtitlesManagerTest)
#include "tst_open_subtitles_manager.moc"
