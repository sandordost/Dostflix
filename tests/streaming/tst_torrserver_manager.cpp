#include "streaming/TorrServerManager.h"

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

namespace {
QByteArray videoTorrentFixture()
{
    constexpr qint64 size = 50LL * 1024 * 1024;
    constexpr qint64 pieceSize = 1024 * 1024;
    const QByteArray hashes(static_cast<qsizetype>((size / pieceSize) * 20), '\0');
    return QByteArray("d4:infod6:lengthi") + QByteArray::number(size)
        + "e4:name10:sample.mp412:piece lengthi" + QByteArray::number(pieceSize)
        + "e6:pieces" + QByteArray::number(hashes.size()) + ':' + hashes + "ee";
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
        manager.setNetworkReady(true);
        QTRY_VERIFY_WITH_TIMEOUT(manager.backendReady(), 10'000);
        QVERIFY(QFileInfo::exists(directory.filePath(
            QStringLiteral("data/torrserver.log"))));
        manager.startTorrentData(QStringLiteral("Fixture"), videoTorrentFixture());
        QTRY_COMPARE_WITH_TIMEOUT(manager.selectedFileName(), QStringLiteral("sample.mp4"), 5'000);
        QVERIFY(manager.active());
        QVERIFY(manager.streamUrl().startsWith(QStringLiteral("http://127.0.0.1:")));
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
};

QTEST_GUILESS_MAIN(TorrServerManagerTest)
#include "tst_torrserver_manager.moc"
