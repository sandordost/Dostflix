#include "streaming/TorrentEngine.h"

#include <QTemporaryDir>
#include <QtTest>

class TorrentEngineTest final : public QObject
{
    Q_OBJECT

private slots:
    void rejectsNetworkWorkBeforeVpnIsReady()
    {
        QTemporaryDir directory;
        TorrentEngine engine(directory.path());
        engine.startMagnet(QStringLiteral("Test"),
                           QStringLiteral("magnet:?xt=urn:btih:0123456789012345678901234567890123456789"));
        QVERIFY(!engine.active());
        QVERIFY(engine.errorMessage().contains(QStringLiteral("VPN")));
    }

    void rejectsUnsupportedAndInvalidPayloadsWithoutOpeningASession()
    {
        QTemporaryDir directory;
        TorrentEngine engine(directory.path());
        engine.setNetworkReady(true);
        engine.startMagnet(QStringLiteral("Test"), QStringLiteral("https://example.invalid/a.torrent"));
        QVERIFY(!engine.active());
        engine.startTorrentData(QStringLiteral("Test"), QByteArrayLiteral("not a torrent"));
        QVERIFY(!engine.active());
        QVERIFY(!engine.errorMessage().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TorrentEngineTest)
#include "tst_torrent_engine.moc"
