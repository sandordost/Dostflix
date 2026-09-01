#include "app/AppSettings.h"
#include "library/LibraryDatabase.h"
#include "library/LibraryManager.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class LibraryManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void scansPersistsAndPlaysLocalVideos()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QString movieDir = root.filePath(QStringLiteral("Movies"));
        QVERIFY(QDir().mkpath(movieDir));
        QFile video(QDir(movieDir).filePath(QStringLiteral("The.Matrix.1999.mkv")));
        QVERIFY(video.open(QIODevice::WriteOnly));
        video.write("local fixture");
        video.close();
        QFile ignored(QDir(movieDir).filePath(QStringLiteral("notes.txt")));
        QVERIFY(ignored.open(QIODevice::WriteOnly));
        ignored.close();

        AppSettings settings(root.filePath(QStringLiteral("settings.ini")));
        LibraryDatabase database(root.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("library-manager-test"));
        QVERIFY(database.open());
        LibraryManager manager(settings, database, movieDir);

        QCOMPARE(manager.count(), 1);
        QCOMPARE(settings.libraryDirectory(), movieDir);
        QCOMPARE(manager.model()->data(manager.model()->index(0, 0),
                                       LocalLibraryModel::TitleRole).toString(),
                 QStringLiteral("The Matrix"));
        QCOMPARE(manager.model()->data(manager.model()->index(0, 0),
                                       LocalLibraryModel::YearRole).toInt(), 1999);
        QSignalSpy playSpy(&manager, &LibraryManager::playbackRequested);
        manager.play(0);
        QCOMPARE(playSpy.size(), 1);
        QCOMPARE(playSpy.first().first().toUrl().toLocalFile(), video.fileName());
        QCOMPARE(playSpy.first().at(2).toInt(), 0);

        manager.recordPlaybackProgress(125, 600, true);
        QCOMPARE(database.movies().first().watchedSeconds, 125);
        QCOMPARE(database.movies().first().durationSeconds, 600);
        manager.clearPlaybackSession();
        manager.recordPlaybackProgress(250, 600, true);
        QCOMPARE(database.movies().first().watchedSeconds, 125);

        manager.play(0);
        QCOMPARE(playSpy.last().at(2).toInt(), 125);
        manager.recordPlaybackProgress(0, 600);
        QCOMPARE(database.movies().first().watchedSeconds, 125);
        manager.play(0, true);
        QCOMPARE(playSpy.last().at(2).toInt(), 0);
        QCOMPARE(playSpy.last().at(0).toUrl().toLocalFile(), video.fileName());
        QCOMPARE(playSpy.last().at(1).toString(), QStringLiteral("The Matrix"));
        QCOMPARE(database.movies().first().watchedSeconds, 0);

        manager.refresh();
        QCOMPARE(manager.count(), 1);
        QCOMPARE(database.movies().size(), 1);
    }

    void rejectsNonLocalDirectory()
    {
        QTemporaryDir root;
        AppSettings settings(root.filePath(QStringLiteral("settings.ini")));
        LibraryDatabase database(root.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("library-manager-invalid-test"));
        QVERIFY(database.open());
        LibraryManager manager(settings, database, root.filePath(QStringLiteral("library")));
        QVERIFY(!manager.setDirectory(QUrl(QStringLiteral("https://example.test/movies"))));
        QVERIFY(!manager.errorMessage().isEmpty());
    }
};

QTEST_GUILESS_MAIN(LibraryManagerTest)
#include "tst_library_manager.moc"
