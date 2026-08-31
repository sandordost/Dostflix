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
};

QTEST_GUILESS_MAIN(AppSettingsTest)
#include "tst_app_settings.moc"
