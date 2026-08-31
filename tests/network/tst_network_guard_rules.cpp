#include "network/NetworkGuardRules.h"

#include <QtTest>

class NetworkGuardRulesTest final : public QObject
{
    Q_OBJECT

    static NetworkGuardRequest validRequest()
    {
        const QString id(32, QLatin1Char('a'));
        const QString scope = QStringLiteral("dostflix-") + id + QStringLiteral(".scope");
        return {id, scope,
                QStringLiteral("user.slice/user-1000.slice/user@1000.service/app.slice/") + scope, 5,
                QHostAddress(QStringLiteral("198.51.100.20")), GuardTransport::Udp,
                443, QStringLiteral("tun0"), GuardPhase::Protected};
    }

private slots:
    void buildsScopedFailClosedRules()
    {
        QString error;
        const QString rules = NetworkGuardRules::build(validRequest(), &error);
        QVERIFY2(!rules.isEmpty(), qPrintable(error));
        QVERIFY(rules.contains(QStringLiteral("socket cgroupv2 level 5")));
        QVERIFY(rules.contains(QStringLiteral("ip daddr 198.51.100.20 udp dport 443 accept")));
        QVERIFY(rules.contains(QStringLiteral("oifname \"tun0\" accept")));
        QVERIFY(rules.contains(QStringLiteral("reject")));
        QVERIFY(rules.startsWith(QStringLiteral("flush table inet dostflix_")));
        QVERIFY(!rules.contains(QStringLiteral("policy drop")));
    }

    void rejectsInjectionInInterfaceName()
    {
        NetworkGuardRequest request = validRequest();
        request.vpnInterface = QStringLiteral("tun0\" accept; flush ruleset");
        QString error;
        QVERIFY(NetworkGuardRules::build(request, &error).isEmpty());
        QVERIFY(!error.isEmpty());
    }

    void bootstrapDoesNotPermitTunnelTraffic()
    {
        NetworkGuardRequest request = validRequest();
        request.phase = GuardPhase::Bootstrap;
        request.vpnInterface.clear();
        QString error;
        const QString rules = NetworkGuardRules::build(request, &error);
        QVERIFY2(!rules.isEmpty(), qPrintable(error));
        QVERIFY(!rules.contains(QStringLiteral("tun0")));
        QVERIFY(!rules.startsWith(QStringLiteral("flush table")));
    }
};

QTEST_GUILESS_MAIN(NetworkGuardRulesTest)
#include "tst_network_guard_rules.moc"
