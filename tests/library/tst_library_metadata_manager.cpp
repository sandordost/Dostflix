#include "app/AppSettings.h"
#include "library/LibraryDatabase.h"
#include "library/LibraryManager.h"
#include "library/LibraryMetadataManager.h"
#include "providers/ProviderManager.h"
#include "providers/SecretStore.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

class MetadataSecretStore final : public SecretStore
{
public:
    bool store(const QString &id, const QString &secret, QString *) override
    { values[id] = secret; return true; }
    QString load(const QString &id, QString *) override { return values.value(id); }
    bool remove(const QString &id, QString *) override { return values.remove(id) > 0; }
    QHash<QString, QString> values;
};

class FakeTmdbServer final : public QObject
{
public:
    explicit FakeTmdbServer(QObject *parent = nullptr) : QObject(parent)
    {
        connect(&server, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *socket = server.nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
                    buffers[socket] += socket->readAll();
                    if (!buffers[socket].contains("\r\n\r\n")) return;
                    const QByteArray request = buffers.take(socket);
                    requests.append(request);
                    QByteArray body;
                    QByteArray type = "application/json";
                    const QByteArray line = request.left(request.indexOf("\r\n"));
                    if (line.startsWith("GET /3/search/movie?"))
                        body = R"({"results":[{"id":603,"title":"The Matrix","release_date":"1999-03-30"}]})";
                    else if (line.startsWith("GET /3/movie/603?"))
                        body = R"({"id":603,"imdb_id":"tt0133093","title":"The Matrix","release_date":"1999-03-30","runtime":136,"overview":"A computer hacker discovers the truth.","poster_path":"/matrix.jpg"})";
                    else if (line.startsWith("GET /w500/matrix.jpg ")) {
                        type = "image/jpeg";
                        body = "fake-jpeg";
                    }
                    const QByteArray response = "HTTP/1.1 200 OK\r\nContent-Type: " + type
                        + "\r\nContent-Length: " + QByteArray::number(body.size())
                        + "\r\nConnection: close\r\n\r\n" + body;
                    socket->write(response);
                    socket->disconnectFromHost();
                });
            }
        });
    }

    bool listen() { return server.listen(QHostAddress::AnyIPv4); }
    QUrl apiBase() const
    { return QUrl(QStringLiteral("http://127.0.0.1:%1/3/").arg(server.serverPort())); }
    QUrl imageBase() const
    { return QUrl(QStringLiteral("http://127.0.0.1:%1/w500/").arg(server.serverPort())); }
    QList<QByteArray> requests;

private:
    QTcpServer server;
    QHash<QTcpSocket *, QByteArray> buffers;
};

class LibraryMetadataManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void enrichesLocalMovieAndCachesPoster()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QString movieDir = root.filePath(QStringLiteral("Movies"));
        QVERIFY(QDir().mkpath(movieDir));
        QFile video(QDir(movieDir).filePath(QStringLiteral("The.Matrix.1999.1080p.mkv")));
        QVERIFY(video.open(QIODevice::WriteOnly));
        video.write("movie");
        video.close();

        FakeTmdbServer server;
        QVERIFY(server.listen());
        AppSettings settings(root.filePath(QStringLiteral("settings.ini")));
        LibraryDatabase database(root.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("metadata-manager-test"));
        QVERIFY(database.open());
        LibraryManager library(settings, database, movieDir);
        MetadataSecretStore secrets;
        secrets.values[QStringLiteral("metadata-tmdb")] = QStringLiteral("read-token");
        ProviderManager providers(settings, secrets);
        LibraryMetadataManager metadata(database, library, providers, root.path(),
                                        server.apiBase(), server.imageBase());

        metadata.setNetworkReady(true);
        QTRY_COMPARE_WITH_TIMEOUT(database.movies().first().tmdbId, 603, 3000);
        const LibraryMovie movie = database.movies().first();
        QCOMPARE(movie.imdbId, QStringLiteral("tt0133093"));
        QCOMPARE(movie.title, QStringLiteral("The Matrix"));
        QCOMPARE(movie.year, 1999);
        QCOMPARE(movie.durationSeconds, 8160);
        QCOMPARE(movie.synopsis, QStringLiteral("A computer hacker discovers the truth."));
        QVERIFY(QFileInfo::exists(movie.posterPath));
        QCOMPARE(server.requests.size(), 3);
        QVERIFY(server.requests.at(0).contains("Authorization: Bearer read-token"));
        QVERIFY(server.requests.at(1).contains("Authorization: Bearer read-token"));
    }
};

QTEST_MAIN(LibraryMetadataManagerTest)
#include "tst_library_metadata_manager.moc"
