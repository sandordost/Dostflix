#include "providers/ReleaseResolver.h"

#include <QtTest>

class ReleaseResolverTest final : public QObject
{
    Q_OBJECT

private slots:
    void acceptsMagnetFromEitherProwlarrField()
    {
        const QString magnet = QStringLiteral("magnet:?xt=urn:btih:abc");
        QCOMPARE(resolveReleaseLocation(magnet, {}).magnetUrl, magnet);
        QCOMPARE(resolveReleaseLocation({}, magnet).magnetUrl, magnet);
    }

    void treatsHttpMagnetFieldAsTorrentDownload()
    {
        const QUrl endpoint(QStringLiteral("http://127.0.0.1:9696/download/42"));
        const ReleaseLocation location = resolveReleaseLocation(endpoint.toString(), {});
        QVERIFY(location.magnetUrl.isEmpty());
        QCOMPARE(location.torrentUrl, endpoint);
    }

    void prefersDedicatedDownloadEndpoint()
    {
        const QUrl download(QStringLiteral("https://indexer.test/download/42"));
        const ReleaseLocation location = resolveReleaseLocation(
            QStringLiteral("https://indexer.test/fallback"), download.toString());
        QCOMPARE(location.torrentUrl, download);
    }
};

QTEST_GUILESS_MAIN(ReleaseResolverTest)
#include "tst_release_resolver.moc"
