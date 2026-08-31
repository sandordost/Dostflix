#include "library/LibraryDatabase.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

class LibraryDatabaseTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsVersionOneSchema()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        LibraryDatabase database(dir.filePath(QStringLiteral("library.sqlite")),
                                 QStringLiteral("test-library"));
        QVERIFY2(database.open(), qPrintable(database.lastError()));
        QCOMPARE(database.schemaVersion(), 1);

        QSqlQuery query(database.connection());
        QVERIFY(query.exec(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE type='table' AND name='movies'")));
        QVERIFY(query.next());
    }
};

QTEST_GUILESS_MAIN(LibraryDatabaseTest)
#include "tst_library_database.moc"
