#include "providers/SecretStore.h"
#include "subtitles/OpenSubtitlesManager.h"

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

    bool listen() { return server.listen(QHostAddress::LocalHost); }
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
            body = R"({"token":"test-token"})";
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
        FakeOpenSubtitlesServer server;
        QVERIFY(server.listen());
        OpenSubtitlesManager manager(secrets, data.path(), server.apiBase());
        QVERIFY(manager.saveCredentials("api-key", "viewer", "secret"));

        manager.search("Test movie");
        QCOMPARE(server.requests.size(), 0);
        QVERIFY(manager.errorMessage().contains("VPN"));
    }

    void searchesDownloadsAndSavesSubtitle()
    {
        MemorySecretStore secrets;
        QTemporaryDir data;
        FakeOpenSubtitlesServer server;
        QVERIFY(server.listen());
        OpenSubtitlesManager manager(secrets, data.path(), server.apiBase());
        QVERIFY(manager.saveCredentials("api-key", "viewer", "secret"));
        QVERIFY(secrets.values.value("subtitles-opensubtitles").contains("api-key"));
        manager.setNetworkReady(true);

        manager.search("Test movie", "nl,en");
        QTRY_COMPARE_WITH_TIMEOUT(manager.results().size(), 1, 3000);
        QCOMPARE(manager.results().first().toMap().value("fileId").toInt(), 42);
        QCOMPARE(manager.results().first().toMap().value("language").toString(), QString("nl"));
        QVERIFY(server.requests.at(0).contains("Api-Key: api-key"));
        QVERIFY(server.requests.at(1).contains("languages=nl,en"));

        QSignalSpy readySpy(&manager, &OpenSubtitlesManager::subtitleReady);
        manager.download(0);
        QTRY_COMPARE_WITH_TIMEOUT(readySpy.size(), 1, 3000);
        QCOMPARE(server.requests.size(), 4);
        QVERIFY(server.requests.at(2).contains("Authorization: Bearer test-token"));
        const QUrl result = readySpy.first().first().toUrl();
        QCOMPARE(result.toLocalFile(), data.path() + "/subtitles/Test.Release.nl.srt");
        QFile file(result.toLocalFile());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll().contains("Hallo"));
    }
};

QTEST_MAIN(OpenSubtitlesManagerTest)
#include "tst_open_subtitles_manager.moc"
