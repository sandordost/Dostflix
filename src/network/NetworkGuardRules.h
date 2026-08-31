#pragma once

#include <QHostAddress>
#include <QString>

enum class GuardTransport { Tcp, Udp };
enum class GuardPhase { Bootstrap, Protected };

struct NetworkGuardRequest
{
    QString sessionId;
    QString scopeName;
    QString cgroupPath;
    int cgroupLevel = 0;
    QHostAddress endpoint;
    GuardTransport transport = GuardTransport::Udp;
    quint16 port = 0;
    QString vpnInterface;
    GuardPhase phase = GuardPhase::Bootstrap;
};

class NetworkGuardRules final
{
public:
    [[nodiscard]] static bool validate(const NetworkGuardRequest &request, QString *error);
    [[nodiscard]] static QString build(const NetworkGuardRequest &request, QString *error);
    [[nodiscard]] static QString tableName(const QString &sessionId);
};
