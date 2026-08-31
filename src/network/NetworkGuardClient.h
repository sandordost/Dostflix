#pragma once

#include "network/NetworkGuardBackend.h"

#include <QHostAddress>

class NetworkGuardClient final : public NetworkGuardBackend
{
public:
    explicit NetworkGuardClient(QString helperPath = {});

    bool installBootstrap(const VpnTransport &transport, QString *error) override;
    bool installProtected(const VpnTransport &transport, const QString &interfaceName,
                          QString *error) override;
    bool remove(QString *error) override;

private:
    bool install(const VpnTransport &transport, const QString &interfaceName,
                 const QString &phase, QString *error);
    bool runHelper(const QStringList &arguments, QString *error);
    bool loadScope(QString *error);

    QString m_helperPath;
    QString m_session;
    QString m_scope;
    int m_cgroupLevel = 0;
    QHostAddress m_endpoint;
};
