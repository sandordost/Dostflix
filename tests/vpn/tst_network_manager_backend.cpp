#include "vpn/NetworkManagerBackend.h"

#include <QtTest>

class NetworkManagerBackendTest final : public QObject
{
    Q_OBJECT

private slots:
    void parsesOnlyVpnProfilesAndUnescapesFields()
    {
        const QByteArray output =
            "1111:AirVPN\\: Netherlands:vpn\n"
            "2222:Wired connection:802-3-ethernet\n"
            "3333:Backslash\\\\ profile:vpn\n";

        const QList<VpnProfile> profiles = NetworkManagerBackend::parseNmcliProfiles(output);

        QCOMPARE(profiles.size(), 2);
        QCOMPARE(profiles.at(0), VpnProfile({QStringLiteral("1111"),
                                            QStringLiteral("AirVPN: Netherlands")}));
        QCOMPARE(profiles.at(1), VpnProfile({QStringLiteral("3333"),
                                            QStringLiteral("Backslash\\ profile")}));
    }

    void parsesOpenVpnTransport()
    {
        QString error;
        const VpnTransport transport = NetworkManagerBackend::parseOpenVpnData(
            QStringLiteral("auth = SHA512, remote = nl3.vpn.example\\:443, proto-tcp = no"),
            &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(transport.host, QStringLiteral("nl3.vpn.example"));
        QCOMPARE(transport.port, quint16(443));
        QVERIFY(!transport.tcp);
    }

    void parsesTheDeviceFromAnIpRoute()
    {
        QCOMPARE(NetworkManagerBackend::parseRouteDevice(
                     "1.1.1.1 via 10.9.6.1 dev tun0 src 10.9.6.231 uid 1000\n"),
                 QStringLiteral("tun0"));
        QVERIFY(NetworkManagerBackend::parseRouteDevice(
                    "RTNETLINK answers: Network is unreachable\n").isEmpty());
    }

    void configuresExclusiveVpnDnsBeforeActivation()
    {
        const QStringList arguments = NetworkManagerBackend::fullTunnelArguments(
            QStringLiteral("vpn-uuid"));
        QCOMPARE(arguments.mid(0, 4), QStringList({QStringLiteral("connection"),
                                                  QStringLiteral("modify"),
                                                  QStringLiteral("uuid"),
                                                  QStringLiteral("vpn-uuid")}));
        const qsizetype ipv4Priority = arguments.indexOf(QStringLiteral("ipv4.dns-priority"));
        const qsizetype ipv6Priority = arguments.indexOf(QStringLiteral("ipv6.dns-priority"));
        QVERIFY(ipv4Priority >= 0);
        QVERIFY(ipv6Priority >= 0);
        QCOMPARE(arguments.at(ipv4Priority + 1), QStringLiteral("-50"));
        QCOMPARE(arguments.at(ipv6Priority + 1), QStringLiteral("-50"));
    }
};

QTEST_GUILESS_MAIN(NetworkManagerBackendTest)
#include "tst_network_manager_backend.moc"
