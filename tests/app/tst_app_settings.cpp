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
            settings.setVpnOwnership(QStringLiteral("vpn-uuid"), 12345);
        }
        AppSettings reloaded(file);
        QCOMPARE(reloaded.libraryDirectory(), QStringLiteral("/media/Movies"));
        QCOMPARE(reloaded.vpnConnectionUuid(), QStringLiteral("vpn-uuid"));
        QCOMPARE(reloaded.ownedVpnConnectionUuid(), QStringLiteral("vpn-uuid"));
        QCOMPARE(reloaded.vpnOwnerPid(), 12345);
        reloaded.clearVpnOwnership();
        QVERIFY(reloaded.ownedVpnConnectionUuid().isEmpty());
        QCOMPARE(reloaded.vpnOwnerPid(), 0);
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
