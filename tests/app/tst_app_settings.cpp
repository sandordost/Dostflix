#include "app/AppSettings.h"
#include <QTemporaryDir>
#include <QtTest>

class AppSettingsTest final : public QObject
{
    Q_OBJECT

private slots:
    void persistsNonSecretPreferences()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString file = dir.filePath(QStringLiteral("settings.ini"));
        {
            AppSettings settings(file);
            settings.setLibraryDirectory(QStringLiteral("/media/Movies"));
            settings.setVpnConnectionUuid(QStringLiteral("vpn-uuid"));
        }
        AppSettings reloaded(file);
        QCOMPARE(reloaded.libraryDirectory(), QStringLiteral("/media/Movies"));
        QCOMPARE(reloaded.vpnConnectionUuid(), QStringLiteral("vpn-uuid"));
    }

    void persistsGenericProviderDefinitions()
    {
        QTemporaryDir dir;
        const QString file = dir.filePath(QStringLiteral("settings.ini"));
        const QList<ProviderConfig> expected{
            {QStringLiteral("provider-1"), QStringLiteral("Home Prowlarr"),
             ProviderKind::Prowlarr, QUrl(QStringLiteral("https://search.example/api/v1")), true},
            {QStringLiteral("provider-2"), QStringLiteral("My Torznab"),
             ProviderKind::Torznab, QUrl(QStringLiteral("https://indexer.example/api")), false}};

        {
            AppSettings settings(file);
            settings.setProviders(expected);
        }

        AppSettings reloaded(file);
        QCOMPARE(reloaded.providers(), expected);
    }
};

QTEST_GUILESS_MAIN(AppSettingsTest)
#include "tst_app_settings.moc"
