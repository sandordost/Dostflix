#include "app/AppSettings.h"
#include "movies/MovieHighlightsManager.h"
#include "providers/ProviderManager.h"
#include "providers/SecretStore.h"

#include <QHash>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

class MemoryHighlightsSecretStore final : public SecretStore
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
    FakeTmdbServer()
    {
        connect(&server, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *socket = server.nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
                    const QByteArray request = socket->readAll();
                    if (!request.contains("\r\n\r\n")) return;
                    requests.append(request);
                    const QByteArray body = R"({"results":[{"id":7,"title":"Arrival","release_date":"2016-11-10","poster_path":"/arrival.jpg","vote_average":8.2}]})";
                    socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
                                  + QByteArray::number(body.size())
                                  + "\r\nConnection: close\r\n\r\n" + body);
                    socket->disconnectFromHost();
                });
            }
        });
    }
    bool listen() { return server.listen(QHostAddress::AnyIPv4); }
    QUrl apiBase() const
    { return QUrl(QStringLiteral("http://127.0.0.1:%1/3/").arg(server.serverPort())); }
    QList<QByteArray> requests;
private:
    QTcpServer server;
};

class MovieHighlightsManagerTest final : public QObject
{
    Q_OBJECT
private slots:
    void staysOfflineUntilVpnAndLoadsAllShelves()
    {
        QTemporaryDir data;
        AppSettings settings(data.filePath(QStringLiteral("settings.ini")));
        MemoryHighlightsSecretStore secrets;
        ProviderManager providers(settings, secrets);
        QVERIFY(providers.saveTmdbToken(QStringLiteral("read-token")));
        FakeTmdbServer server;
        QVERIFY(server.listen());
        MovieHighlightsManager manager(providers, server.apiBase());
        manager.refresh();
        QCOMPARE(server.requests.size(), 0);

        manager.setNetworkReady(true);
        QTRY_COMPARE_WITH_TIMEOUT(server.requests.size(), 3, 3000);
        QTRY_VERIFY_WITH_TIMEOUT(!manager.busy(), 3000);
        QCOMPARE(manager.trendingModel()->rowCount(), 1);
        QCOMPARE(manager.bestOfYearModel()->rowCount(), 1);
        QCOMPARE(manager.highRatingsModel()->rowCount(), 1);
        QCOMPARE(manager.trendingModel()->data(manager.trendingModel()->index(0, 0),
                 MovieListModel::TitleRole).toString(), QStringLiteral("Arrival"));
        QCOMPARE(manager.trendingModel()->data(manager.trendingModel()->index(0, 0),
                 MovieListModel::RatingRole).toDouble(), 8.2);
        QVERIFY(server.requests.first().contains("Authorization: Bearer read-token"));
    }
};

QTEST_GUILESS_MAIN(MovieHighlightsManagerTest)
#include "tst_movie_highlights_manager.moc"
