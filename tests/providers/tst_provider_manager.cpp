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
    bool store(const QString &id, const QString &secret, QString *error) override
    { storedId = id; storedSecret = secret; error->clear(); return true; }
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
};

QTEST_GUILESS_MAIN(ProviderManagerTest)
#include "tst_provider_manager.moc"
