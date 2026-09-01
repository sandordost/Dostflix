#include "streaming/StreamServer.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest>

class StreamServerTest final : public QObject
{
    Q_OBJECT

private slots:
    void servesSeekableByteRangesOnLoopback()
    {
        QTemporaryFile file;
        QVERIFY(file.open());
        QCOMPARE(file.write("0123456789abcdef"), qint64(16));
        file.flush();

        StreamServer server;
        QVERIFY(server.start(file.fileName(), 16, [](qint64, qint64) { return true; }));
        QVERIFY(server.url().startsWith(QStringLiteral("http://127.0.0.1:")));

        QNetworkRequest request(QUrl(server.url()));
        request.setRawHeader("Range", "bytes=5-9");
        QNetworkAccessManager network;
        QNetworkReply *reply = network.get(request);
        QSignalSpy finished(reply, &QNetworkReply::finished);
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 2'000);
        QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 206);
        QCOMPARE(reply->readAll(), QByteArray("56789"));
        reply->deleteLater();
    }

    void rejectsUnguessableWrongPath()
    {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write("data");
        file.flush();
        StreamServer server;
        QVERIFY(server.start(file.fileName(), 4, [](qint64, qint64) { return true; }));
        const QUrl valid(server.url());
        const QUrl wrong(QStringLiteral("http://127.0.0.1:%1/stream/wrong")
                             .arg(valid.port()));
        QNetworkAccessManager network;
        QNetworkReply *reply = network.get(QNetworkRequest(wrong));
        QSignalSpy finished(reply, &QNetworkReply::finished);
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 2'000);
        QCOMPARE(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), 404);
        reply->deleteLater();
    }

    void waitsForVerifiedBytesAndRequestsPriority()
    {
        QTemporaryFile file;
        QVERIFY(file.open());
        QCOMPARE(file.write("0123456789abcdef"), qint64(16));
        file.flush();

        bool available = false;
        int priorityRequests = 0;
        StreamServer server;
        QVERIFY(server.start(file.fileName(), 16,
                             [&](qint64, qint64) { return available; },
                             [&](qint64, qint64) { ++priorityRequests; }));

        QNetworkRequest request(QUrl(server.url()));
        request.setRawHeader("Range", "bytes=8-11");
        QNetworkAccessManager network;
        QNetworkReply *reply = network.get(request);
        QSignalSpy finished(reply, &QNetworkReply::finished);
        QTest::qWait(150);
        QCOMPARE(finished.count(), 0);
        QVERIFY(priorityRequests > 0);

        available = true;
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 2'000);
        QCOMPARE(reply->readAll(), QByteArray("89ab"));
        reply->deleteLater();
    }
};

QTEST_GUILESS_MAIN(StreamServerTest)
#include "tst_stream_server.moc"
