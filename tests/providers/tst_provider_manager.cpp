#include "app/AppSettings.h"
#include "providers/ProviderManager.h"
#include "providers/SecretStore.h"

#include <QTemporaryDir>
#include <QtTest>

class FakeSecretStore final : public SecretStore
{
public:
    QString storedId;
    QString storedSecret;
    QString removedId;
    QString loadedSecret;
    bool store(const QString &id, const QString &secret, QString *error) override
    { storedId = id; storedSecret = secret; error->clear(); return true; }
    QString load(const QString &, QString *error) override
    { error->clear(); return loadedSecret; }
    bool remove(const QString &id, QString *error) override
    { removedId = id; error->clear(); return true; }
};

class ProviderManagerTest final : public QObject
{
    Q_OBJECT
private slots:
    void addsAndRemovesAUserProvider()
    {
        QTemporaryDir directory;
        AppSettings settings(directory.filePath(QStringLiteral("settings.ini")));
        FakeSecretStore secrets;
        ProviderManager manager(settings, secrets);

        QVERIFY(manager.addProvider(QStringLiteral("My Torznab"), QStringLiteral("Torznab"),
                                    QStringLiteral("https://indexer.example/api"),
                                    QStringLiteral("secret-key")));
        QCOMPARE(manager.model()->rowCount(), 1);
        QCOMPARE(settings.providers().size(), 1);
        QCOMPARE(secrets.storedSecret, QStringLiteral("secret-key"));
        QVERIFY(!secrets.storedId.isEmpty());

        manager.removeProvider(0);
        QCOMPARE(manager.model()->rowCount(), 0);
        QCOMPARE(secrets.removedId, secrets.storedId);
    }

    void storesTmdbTokenOutsideSettings()
    {
        QTemporaryDir directory;
        AppSettings settings(directory.filePath(QStringLiteral("settings.ini")));
        FakeSecretStore secrets;
        ProviderManager manager(settings, secrets);

        QVERIFY(manager.saveTmdbToken(QStringLiteral(" bearer-token ")));
        QVERIFY(manager.hasTmdbToken());
        QCOMPARE(manager.tmdbToken(), QStringLiteral("bearer-token"));
        QCOMPARE(secrets.storedId, QStringLiteral("metadata-tmdb"));
        manager.clearTmdbToken();
        QVERIFY(!manager.hasTmdbToken());
    }
};

QTEST_GUILESS_MAIN(ProviderManagerTest)
#include "tst_provider_manager.moc"
