#include "network/NetworkGuardRules.h"

#include <QRegularExpression>

namespace {
const QRegularExpression sessionPattern(QStringLiteral("^[a-f0-9]{32}$"));
const QRegularExpression scopePattern(QStringLiteral("^dostflix-[a-f0-9]{32}\\.scope$"));
const QRegularExpression interfacePattern(QStringLiteral("^[A-Za-z0-9_.-]{1,15}$"));
}

QString NetworkGuardRules::tableName(const QString &sessionId)
{
    return QStringLiteral("dostflix_") + sessionId;
}

bool NetworkGuardRules::validate(const NetworkGuardRequest &request, QString *error)
{
    auto reject = [error](const QString &message) {
        if (error != nullptr) {
            *error = message;
        }
        return false;
    };
    if (!sessionPattern.match(request.sessionId).hasMatch()) {
        return reject(QStringLiteral("Invalid guard session identifier"));
    }
    if (!scopePattern.match(request.scopeName).hasMatch()
        || !request.scopeName.contains(request.sessionId)) {
        return reject(QStringLiteral("Invalid Dostflix cgroup scope"));
    }
    if (request.cgroupLevel < 1 || request.cgroupLevel > 16) {
        return reject(QStringLiteral("Invalid cgroup level"));
    }
    if (request.endpoint.isNull()
        || request.endpoint.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol) {
        return reject(QStringLiteral("VPN endpoint must be a numeric IPv4 or IPv6 address"));
    }
    if (request.port == 0) {
        return reject(QStringLiteral("VPN endpoint port is invalid"));
    }
    if (request.phase == GuardPhase::Protected
        && !interfacePattern.match(request.vpnInterface).hasMatch()) {
        return reject(QStringLiteral("VPN interface name is invalid"));
    }
    return true;
}

QString NetworkGuardRules::build(const NetworkGuardRequest &request, QString *error)
{
    if (!validate(request, error)) {
        return {};
    }
    const QString match = QStringLiteral("socket cgroupv2 level %1 \"%2\"")
                              .arg(request.cgroupLevel).arg(request.scopeName);
    const QString family = request.endpoint.protocol() == QAbstractSocket::IPv4Protocol
                               ? QStringLiteral("ip") : QStringLiteral("ip6");
    const QString transport = request.transport == GuardTransport::Udp
                                  ? QStringLiteral("udp") : QStringLiteral("tcp");
    QString rules = QStringLiteral(
        "table inet %1 {\n"
        "  chain output {\n"
        "    type filter hook output priority -10; policy accept;\n"
        "    %2 oifname \"lo\" accept\n"
        "    %2 %3 daddr %4 %5 dport %6 accept\n")
        .arg(tableName(request.sessionId), match, family,
             request.endpoint.toString(), transport, QString::number(request.port));
    if (request.phase == GuardPhase::Protected) {
        rules += QStringLiteral("    %1 oifname \"%2\" accept\n")
                     .arg(match, request.vpnInterface);
    }
    rules += QStringLiteral("    %1 reject\n  }\n}\n").arg(match);
    return rules;
}
