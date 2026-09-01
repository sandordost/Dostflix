#include "library/LibraryDatabase.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

class LibraryDatabaseTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsCurrentSchema()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        LibraryDatabase database(dir.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("test-library"));
        QVERIFY2(database.open(), qPrintable(database.lastError()));
        QCOMPARE(database.schemaVersion(), 4);

        QSqlQuery query(database.connection());
        QVERIFY(query.exec(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE type='table' AND name='movies'")));
        QVERIFY(query.next());
    }

    void upsertsMoviesByVideoPath()
    {
        QTemporaryDir dir;
        LibraryDatabase database(dir.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("test-library-upsert"));
        QVERIFY2(database.open(), qPrintable(database.lastError()));
        QVERIFY(database.upsertMovie(QStringLiteral("First title"),
                                     QStringLiteral("/movies/test.mkv")));
        QVERIFY(database.upsertMovie(QStringLiteral("Updated title"),
                                     QStringLiteral("/movies/test.mkv")));
        const QList<LibraryMovie> movies = database.movies();
        QCOMPARE(movies.size(), 1);
        QCOMPARE(movies.first().title, QStringLiteral("Updated title"));
        QCOMPARE(movies.first().videoPath, QStringLiteral("/movies/test.mkv"));
        QVERIFY(database.updateMovieMetadata(QStringLiteral("/movies/test.mkv"), 603,
            QStringLiteral("tt0133093"), QStringLiteral("The Matrix"), 1999,
            QStringLiteral("/cache/603.jpg"), 8160,
            QStringLiteral("A computer hacker discovers the truth.")));
        const LibraryMovie enriched = database.movies().first();
        QCOMPARE(enriched.tmdbId, 603);
        QCOMPARE(enriched.imdbId, QStringLiteral("tt0133093"));
        QCOMPARE(enriched.title, QStringLiteral("The Matrix"));
        QCOMPARE(enriched.year, 1999);
        QCOMPARE(enriched.posterPath, QStringLiteral("/cache/603.jpg"));
        QCOMPARE(enriched.durationSeconds, 8160);
        QCOMPARE(enriched.synopsis, QStringLiteral("A computer hacker discovers the truth."));
        QVERIFY(database.updateWatchProgress(QStringLiteral("/movies/test.mkv"), 321, 8200));
        const LibraryMovie progressed = database.movies().first();
        QCOMPARE(progressed.watchedSeconds, 321);
        QCOMPARE(progressed.durationSeconds, 8200);
    }

    void persistsResumableTransferState()
    {
        QTemporaryDir dir;
        LibraryDatabase database(dir.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("test-library-transfer"));
        QVERIFY(database.open());
        LibraryTransfer transfer{QStringLiteral("abcdef"), 3, QStringLiteral("Movie"),
            QStringLiteral("movie.mkv"), 100, QStringLiteral("/movies/movie.part"),
            QStringLiteral("/movies/movie.mkv"), 25, QStringLiteral("paused")};
        QVERIFY(database.saveTransfer(transfer));
        QVERIFY(database.updateTransferProgress(QStringLiteral("abcdef"), 3, 50,
                                                QStringLiteral("downloading")));
        const auto stored = database.latestIncompleteTransfer();
        QVERIFY(stored.has_value());
        QCOMPARE(stored->bytesWritten, 50);
        QCOMPARE(stored->fileIndex, 3);
        QCOMPARE(database.transfer(QStringLiteral("abcdef"), 3)->state,
                 QStringLiteral("downloading"));
        QVERIFY(database.latestTransfer().has_value());
        QVERIFY(database.removeTransfer(QStringLiteral("abcdef"), 3));
        QVERIFY(!database.transfer(QStringLiteral("abcdef"), 3).has_value());
    }
};

QTEST_GUILESS_MAIN(LibraryDatabaseTest)
#include "tst_library_database.moc"
