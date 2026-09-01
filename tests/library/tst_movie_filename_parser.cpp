#include "library/MovieFilenameParser.h"

#include <QtTest>

class MovieFilenameParserTest final : public QObject
{
    Q_OBJECT

private slots:
    void recognizesReleaseNames_data()
    {
        QTest::addColumn<QString>("fileName");
        QTest::addColumn<QString>("title");
        QTest::addColumn<int>("year");
        QTest::newRow("year and bluray") << "The.Matrix.1999.1080p.BluRay.x264.mkv"
                                          << "The Matrix" << 1999;
        QTest::newRow("web release") << "Obsession.2026.1080p.AMZN.WEB-DL.DDP5.1.H264.MP4-BTM.mp4"
                                      << "Obsession" << 2026;
        QTest::newRow("bracketed year") << "Blade_Runner_2049_(2017)_[2160p].mkv"
                                         << "Blade Runner 2049" << 2017;
        QTest::newRow("no year") << "Arrival.1080p.WEB-DL.H264.mkv" << "Arrival" << 0;
    }

    void recognizesReleaseNames()
    {
        QFETCH(QString, fileName);
        QFETCH(QString, title);
        QFETCH(int, year);
        const ParsedMovieFilename parsed = parseMovieFilename(fileName);
        QCOMPARE(parsed.title, title);
        QCOMPARE(parsed.year, year);
    }
};

QTEST_GUILESS_MAIN(MovieFilenameParserTest)
#include "tst_movie_filename_parser.moc"
