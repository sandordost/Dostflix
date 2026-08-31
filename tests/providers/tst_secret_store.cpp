#include "providers/SecretStore.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class SecretStoreTest final : public QObject
{
    Q_OBJECT

private slots:
    void storesLoadsAndRemovesWithOwnerOnlyPermissions()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        qputenv("XDG_CONFIG_HOME", directory.path().toUtf8());

        LibSecretStore store;
        QString error;
        QVERIFY2(store.store(QStringLiteral("tmdb"), QStringLiteral("test-token"), &error),
                 qPrintable(error));
        QCOMPARE(store.load(QStringLiteral("tmdb"), &error), QStringLiteral("test-token"));
        QVERIFY2(error.isEmpty(), qPrintable(error));

        const QString path = directory.filePath(QStringLiteral("dostflix/credentials.ini"));
        const QFileDevice::Permissions permissions = QFile::permissions(path);
        QVERIFY(permissions.testFlag(QFileDevice::ReadOwner));
        QVERIFY(permissions.testFlag(QFileDevice::WriteOwner));
        QVERIFY(!(permissions & (QFileDevice::ReadGroup | QFileDevice::WriteGroup
                                 | QFileDevice::ReadOther | QFileDevice::WriteOther)));
        QVERIFY(store.remove(QStringLiteral("tmdb"), &error));
        QVERIFY(store.load(QStringLiteral("tmdb"), &error).isEmpty());
    }
};

QTEST_GUILESS_MAIN(SecretStoreTest)
#include "tst_secret_store.moc"
