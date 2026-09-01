#include "streaming/TorrServerManager.h"

#include <QFileInfo>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTemporaryDir>
#include <QUrlQuery>
#include <QtTest>

namespace {
QByteArray videoTorrentFixture(QByteArray name = QByteArrayLiteral("sample.mp4"), char hashByte = '\0')
{
    constexpr qint64 size = 50LL * 1024 * 1024;
    constexpr qint64 pieceSize = 1024 * 1024;
    const QByteArray hashes(static_cast<qsizetype>((size / pieceSize) * 20), hashByte);
    return QByteArray("d4:infod6:lengthi") + QByteArray::number(size)
        + "e4:name" + QByteArray::number(name.size()) + ':' + name
        + "12:piece lengthi" + QByteArray::number(pieceSize)
        + "e6:pieces" + QByteArray::number(hashes.size()) + ':' + hashes + "ee";
}

QJsonArray listTorrents(const QUrl &streamUrl)
{
    QUrl url = streamUrl;
    url.setPath(QStringLiteral("/torrents"));
    url.setQuery(QString());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setTransferTimeout(3'000);
    QNetworkAccessManager network;
    QNetworkReply *reply = network.post(
        request, QJsonDocument(QJsonObject{{QStringLiteral("action"), QStringLiteral("list")}})
                     .toJson(QJsonDocument::Compact));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    const QJsonArray result = QJsonDocument::fromJson(reply->readAll()).array();
    reply->deleteLater();
    return result;
}

int activeTorrentCount(const QJsonArray &torrents)
{
    int active = 0;
    for (const QJsonValue &value : torrents) {
        // TorrServer status 5 is a persisted database entry with no live swarm.
        if (value.toObject().value(QStringLiteral("stat")).toInt() != 5) ++active;
    }
    return active;
}
}

class TorrServerManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void rejectsTorrentTrafficBeforeVpnProtection()
    {
        QTemporaryDir directory;
        TorrServerManager manager(directory.filePath(QStringLiteral("data")));
        manager.startMagnet(QStringLiteral("Test"),
                            QStringLiteral("magnet:?xt=urn:btih:0123456789012345678901234567890123456789"));
        QVERIFY(!manager.active());
        QVERIFY(manager.errorMessage().contains(QStringLiteral("VPN")));
    }

    void startsManagedLoopbackBackendAndSelectsVideo()
    {
        QTemporaryDir directory;
        TorrServerManager manager(directory.filePath(QStringLiteral("data")));
        QSignalSpy retentionSpy(&manager, &TorrServerManager::retentionSourceReady);
        manager.setNetworkReady(true);
        QTRY_VERIFY_WITH_TIMEOUT(manager.backendReady(), 10'000);
        QVERIFY(QFileInfo::exists(directory.filePath(
            QStringLiteral("data/torrserver.log"))));
        manager.startTorrentData(QStringLiteral("Fixture"), videoTorrentFixture());
        QTRY_COMPARE_WITH_TIMEOUT(manager.selectedFileName(), QStringLiteral("sample.mp4"), 5'000);
        QTRY_COMPARE_WITH_TIMEOUT(retentionSpy.size(), 1, 5'000);
        QVERIFY(manager.active());
        QVERIFY(manager.streamUrl().startsWith(QStringLiteral("http://127.0.0.1:")));
        QTest::qWait(16'000);
        QVERIFY2(manager.active(), qPrintable(manager.errorMessage()));
        QVERIFY(manager.errorMessage().isEmpty());
        manager.shutdown();
        QVERIFY(!manager.backendReady());
    }

    void magnetWithoutPeersRemainsInMetadataAcquisition()
    {
        QTemporaryDir directory;
        TorrServerManager manager(directory.filePath(QStringLiteral("data")));
        manager.setNetworkReady(true);
        QTRY_VERIFY_WITH_TIMEOUT(manager.backendReady(), 10'000);
        manager.startMagnet(
            QStringLiteral("Metadata pending"),
            QStringLiteral("magnet:?xt=urn:btih:0123456789012345678901234567890123456789"));
        QTest::qWait(1'500);
        QVERIFY(manager.active());
        QVERIFY(manager.errorMessage().isEmpty());
    }

    void keepsOnlyOneActiveTorrentWhenReplacing()
    {
        QTemporaryDir directory;
        TorrServerManager manager(directory.filePath(QStringLiteral("data")));
        manager.setNetworkReady(true);
        QTRY_VERIFY_WITH_TIMEOUT(manager.backendReady(), 10'000);

        manager.startTorrentData(QStringLiteral("First"),
                                 videoTorrentFixture(QByteArrayLiteral("firstx.mp4"), '\1'));
        QTRY_COMPARE_WITH_TIMEOUT(manager.selectedFileName(), QStringLiteral("firstx.mp4"), 5'000);
        QCOMPARE(activeTorrentCount(listTorrents(QUrl(manager.streamUrl()))), 1);

        manager.startTorrentData(QStringLiteral("Second"),
                                 videoTorrentFixture(QByteArrayLiteral("second.mp4"), '\2'));
        QTRY_COMPARE_WITH_TIMEOUT(manager.selectedFileName(), QStringLiteral("second.mp4"), 5'000);
        QCOMPARE(manager.title(), QStringLiteral("Second"));
        const QJsonArray torrents = listTorrents(QUrl(manager.streamUrl()));
        QCOMPARE(torrents.size(), 2);
        QCOMPARE(activeTorrentCount(torrents), 1);
    }

    void cancelClearsUiStateWithoutDeletingCachedTorrent()
    {
        QTemporaryDir directory;
        TorrServerManager manager(directory.filePath(QStringLiteral("data")));
        manager.setNetworkReady(true);
        QTRY_VERIFY_WITH_TIMEOUT(manager.backendReady(), 10'000);
        manager.startTorrentData(QStringLiteral("Cancelable"), videoTorrentFixture());
        QTRY_COMPARE_WITH_TIMEOUT(manager.selectedFileName(), QStringLiteral("sample.mp4"), 5'000);
        const QUrl apiUrl(manager.streamUrl());

        manager.cancel();
        QTest::qWait(1'200);
        QVERIFY(!manager.active());
        QVERIFY(manager.title().isEmpty());
        QVERIFY(manager.stateLabel().isEmpty());
        QCOMPARE(manager.progress(), 0.0);
        QVERIFY(!listTorrents(apiUrl).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TorrServerManagerTest)
#include "tst_torrserver_manager.moc"
