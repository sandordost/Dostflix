#include "app/AppPaths.h"
#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

class AppPathsTest final : public QObject
{
    Q_OBJECT

private slots:
    void usesInjectedXdgRoots()
    {
        const AppPaths paths(QStringLiteral("/tmp/dostflix-config"),
                             QStringLiteral("/tmp/dostflix-data"),
                             QStringLiteral("/tmp/dostflix-cache"));
        QCOMPARE(paths.configDir(), QStringLiteral("/tmp/dostflix-config/dostflix"));
        QCOMPARE(paths.dataDir(), QStringLiteral("/tmp/dostflix-data/dostflix"));
        QCOMPARE(paths.cacheDir(), QStringLiteral("/tmp/dostflix-cache/dostflix"));
    }

    void createsAllApplicationDirectories()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const AppPaths paths(root.filePath(QStringLiteral("config")),
                             root.filePath(QStringLiteral("data")),
                             root.filePath(QStringLiteral("cache")));
        QVERIFY(paths.ensureExists());
        QVERIFY(QDir(paths.configDir()).exists());
        QVERIFY(QDir(paths.dataDir()).exists());
        QVERIFY(QDir(paths.cacheDir()).exists());
    }
};

QTEST_GUILESS_MAIN(AppPathsTest)
#include "tst_app_paths.moc"
