#include "providers/TorznabParser.h"

#include <QtTest>

class TorznabParserTest final : public QObject
{
    Q_OBJECT

private slots:
    void parsesNormalizedMovieReleases()
    {
        const QByteArray xml =
            "<?xml version=\"1.0\"?>"
            "<rss xmlns:torznab=\"http://torznab.com/schemas/2015/feed\"><channel><item>"
            "<title>Example Movie 2026 1080p</title>"
            "<link>https://indexer.test/download/42</link>"
            "<pubDate>Mon, 31 Aug 2026 20:00:00 +0000</pubDate>"
            "<enclosure url=\"https://indexer.test/download/42\" length=\"4294967296\" "
            "type=\"application/x-bittorrent\"/>"
            "<torznab:attr name=\"seeders\" value=\"81\"/>"
            "<torznab:attr name=\"peers\" value=\"93\"/>"
            "<torznab:attr name=\"category\" value=\"2000\"/>"
            "<torznab:attr name=\"magneturl\" value=\"magnet:?xt=urn:btih:abc123\"/>"
            "</item></channel></rss>";

        QString error;
        const QList<ProviderRelease> releases = TorznabParser::parse(xml, QStringLiteral("My provider"), &error);

        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(releases.size(), 1);
        const ProviderRelease &release = releases.constFirst();
        QCOMPARE(release.title, QStringLiteral("Example Movie 2026 1080p"));
        QCOMPARE(release.sourceLabel, QStringLiteral("My provider"));
        QCOMPARE(release.downloadUrl, QUrl(QStringLiteral("https://indexer.test/download/42")));
        QCOMPARE(release.magnetUrl, QStringLiteral("magnet:?xt=urn:btih:abc123"));
        QCOMPARE(release.sizeBytes, qint64(4'294'967'296));
        QCOMPARE(release.seeders, 81);
        QCOMPARE(release.leechers, 12);
        QCOMPARE(release.category, QStringLiteral("2000"));
        QVERIFY(release.publishedAt.isValid());
    }

    void rejectsInvalidXml()
    {
        QString error;
        QVERIFY(TorznabParser::parse("<rss><item>", QStringLiteral("Broken"), &error).isEmpty());
        QVERIFY(!error.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TorznabParserTest)
#include "tst_torznab_parser.moc"
