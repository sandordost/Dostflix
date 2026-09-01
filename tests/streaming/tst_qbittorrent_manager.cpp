#include "streaming/QBitTorrentManager.h"

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

class QBitTorrentManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void rejectsTorrentTrafficBeforeVpnProtection()
    {
        QTemporaryDir directory;
        QBitTorrentManager manager(directory.filePath(QStringLiteral("data")),
                                   directory.filePath(QStringLiteral("downloads")));

        manager.startMagnet(QStringLiteral("Test"),
                            QStringLiteral("magnet:?xt=urn:btih:0123456789012345678901234567890123456789"));

        QVERIFY(!manager.active());
        QVERIFY(manager.errorMessage().contains(QStringLiteral("VPN")));
    }

    void rejectsInvalidRangesWithoutASelectedTorrent()
    {
        QTemporaryDir directory;
        QBitTorrentManager manager(directory.filePath(QStringLiteral("data")),
                                   directory.filePath(QStringLiteral("downloads")));
        QVERIFY(!manager.isRangeAvailable(0, 1024));
    }

    void startsManagedLoopbackBackend()
    {
        QTemporaryDir directory;
        QBitTorrentManager manager(directory.filePath(QStringLiteral("data")),
                                   directory.filePath(QStringLiteral("downloads")));
        manager.setNetworkReady(true);
        QTRY_VERIFY_WITH_TIMEOUT(manager.backendReady(), 10'000);
        manager.startTorrentData(QStringLiteral("Fixture"), videoTorrentFixture());
        QTRY_COMPARE_WITH_TIMEOUT(manager.selectedFileName(), QStringLiteral("sample.mp4"), 5'000);
        QVERIFY(manager.active());
        manager.shutdown();
        QVERIFY(!manager.backendReady());
    }

    void keepsWaitingWhileMagnetMetadataIsUnavailable()
    {
        QTemporaryDir directory;
        QBitTorrentManager manager(directory.filePath(QStringLiteral("data")),
                                   directory.filePath(QStringLiteral("downloads")));
        manager.setNetworkReady(true);
        QTRY_VERIFY_WITH_TIMEOUT(manager.backendReady(), 10'000);

        manager.startMagnet(
            QStringLiteral("Metadata pending"),
            QStringLiteral("magnet:?xt=urn:btih:0123456789012345678901234567890123456789"));
        QTest::qWait(1'500);

        QVERIFY(manager.active());
        QVERIFY(manager.errorMessage().isEmpty());
        QVERIFY(manager.stateLabel().contains(QStringLiteral("metadata"),
                                              Qt::CaseInsensitive));
    }
};

QTEST_GUILESS_MAIN(QBitTorrentManagerTest)
#include "tst_qbittorrent_manager.moc"
