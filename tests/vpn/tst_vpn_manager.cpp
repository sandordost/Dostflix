#include "app/AppSettings.h"
#include "network/NetworkGuardBackend.h"
#include "vpn/VpnBackend.h"
#include "vpn/VpnManager.h"

#include <QTemporaryDir>
#include <QtTest>

class FakeVpnBackend final : public VpnBackend
{
public:
    QList<VpnProfile> availableProfiles{{QStringLiteral("vpn-1"), QStringLiteral("Test VPN")}};
    VpnConnectionState state = VpnConnectionState::Inactive;
    QString activePath = QStringLiteral("/active/1");
    int activationCount = 0;
    int deactivationCount = 0;
    QString deactivatedPath;

    QList<VpnProfile> profiles(QString *error) override
    {
        error->clear();
        return availableProfiles;
    }

    QString importOpenVpn(const QString &, QString *error) override
    {
        error->clear();
        return QStringLiteral("vpn-1");
    }

    VpnTransport transport(const QString &, QString *error) override
    {
        error->clear();
        return {QStringLiteral("vpn.example.test"), 443, false};
    }

    QString tunnelInterface(const QString &, QString *error) override
    {
        error->clear();
        return QStringLiteral("tun0");
    }

    bool routeUsesInterface(const QString &, QString *error) override
    {
        error->clear();
        return true;
    }

    bool dnsUsesInterface(const QString &, QString *error) override
    {
        error->clear();
        return true;
    }

    VpnConnectionState connectionState(const QString &, QString *path, QString *error) override
    {
        error->clear();
        if (state != VpnConnectionState::Inactive) {
            *path = activePath;
        }
        return state;
    }

    bool activate(const QString &, QString *path, QString *error) override
    {
        error->clear();
        ++activationCount;
        *path = activePath;
        state = VpnConnectionState::Activating;
        return true;
    }

    bool deactivate(const QString &path, QString *error) override
    {
        error->clear();
        deactivatedPath = path;
        ++deactivationCount;
        state = VpnConnectionState::Inactive;
        return true;
    }
};

class FakeNetworkGuard final : public NetworkGuardBackend
{
public:
    int bootstrapCount = 0;
    int protectedCount = 0;
    int removeCount = 0;

    bool installBootstrap(const VpnTransport &, QString *error) override
    {
        error->clear();
        ++bootstrapCount;
        return true;
    }
    bool installProtected(const VpnTransport &, const QString &interfaceName,
                          QString *error) override
    {
        error->clear();
        if (interfaceName != QStringLiteral("tun0")) return false;
        ++protectedCount;
        return true;
    }
    bool remove(QString *error) override
    {
        error->clear();
        ++removeCount;
        return true;
    }
};

class VpnManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void startsAndStopsAnOwnedConnection()
    {
        QTemporaryDir directory;
        AppSettings settings(directory.filePath(QStringLiteral("settings.ini")));
        settings.setVpnConnectionUuid(QStringLiteral("vpn-1"));
        FakeVpnBackend backend;
        VpnManager manager(settings, backend);

        manager.start();
        QCOMPARE(backend.activationCount, 1);
        QVERIFY(manager.ownsConnection());
        QVERIFY(manager.busy());

        manager.shutdown();
        QCOMPARE(backend.deactivationCount, 1);
        QCOMPARE(backend.deactivatedPath, backend.activePath);
        QVERIFY(!manager.ownsConnection());
    }

    void neverStopsAPreexistingConnection()
    {
        QTemporaryDir directory;
        AppSettings settings(directory.filePath(QStringLiteral("settings.ini")));
        settings.setVpnConnectionUuid(QStringLiteral("vpn-1"));
        FakeVpnBackend backend;
        backend.state = VpnConnectionState::Activated;
        VpnManager manager(settings, backend);

        manager.start();
        QVERIFY(manager.connected());
        QVERIFY(!manager.ownsConnection());

        manager.shutdown();
        QCOMPARE(backend.deactivationCount, 0);
    }

    void stopsOwnedConnectionBeforeChangingProfile()
    {
        QTemporaryDir directory;
        AppSettings settings(directory.filePath(QStringLiteral("settings.ini")));
        settings.setVpnConnectionUuid(QStringLiteral("vpn-1"));
        FakeVpnBackend backend;
        VpnManager manager(settings, backend);
        manager.start();

        manager.selectProfile(QStringLiteral("vpn-2"));

        QCOMPARE(backend.deactivationCount, 1);
        QCOMPARE(manager.selectedProfileUuid(), QStringLiteral("vpn-2"));
        QVERIFY(!manager.ownsConnection());
    }

    void clearsASelectionThatNoLongerExists()
    {
        QTemporaryDir directory;
        AppSettings settings(directory.filePath(QStringLiteral("settings.ini")));
        settings.setVpnConnectionUuid(QStringLiteral("missing"));
        FakeVpnBackend backend;
        VpnManager manager(settings, backend);

        manager.refreshProfiles();

        QVERIFY(manager.selectedProfileUuid().isEmpty());
        QVERIFY(settings.vpnConnectionUuid().isEmpty());
    }

    void exposesNetworkingOnlyAfterProtectedGuard()
    {
        QTemporaryDir directory;
        AppSettings settings(directory.filePath(QStringLiteral("settings.ini")));
        settings.setVpnConnectionUuid(QStringLiteral("vpn-1"));
        FakeVpnBackend backend;
        backend.state = VpnConnectionState::Activated;
        FakeNetworkGuard guard;
        VpnManager manager(settings, backend, &guard);

        manager.start();

        QCOMPARE(guard.bootstrapCount, 1);
        QCOMPARE(guard.protectedCount, 1);
        QVERIFY(manager.networkReady());
        manager.shutdown();
        QCOMPARE(guard.removeCount, 1);
        QCOMPARE(backend.deactivationCount, 0);
    }

    void removesGuardWhenSwitchingFromExternalVpn()
    {
        QTemporaryDir directory;
        AppSettings settings(directory.filePath(QStringLiteral("settings.ini")));
        settings.setVpnConnectionUuid(QStringLiteral("vpn-1"));
        FakeVpnBackend backend;
        backend.state = VpnConnectionState::Activated;
        FakeNetworkGuard guard;
        VpnManager manager(settings, backend, &guard);
        manager.start();

        manager.selectProfile(QStringLiteral("vpn-2"));

        QCOMPARE(guard.removeCount, 1);
        QCOMPARE(backend.deactivationCount, 0);
        QCOMPARE(manager.selectedProfileUuid(), QStringLiteral("vpn-2"));
    }
};

QTEST_GUILESS_MAIN(VpnManagerTest)
#include "tst_vpn_manager.moc"
