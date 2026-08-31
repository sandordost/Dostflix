#pragma once

#include "vpn/VpnBackend.h"

class NetworkGuardBackend
{
public:
    virtual ~NetworkGuardBackend() = default;
    virtual bool installBootstrap(const VpnTransport &transport, QString *error) = 0;
    virtual bool installProtected(const VpnTransport &transport, const QString &interfaceName,
                                  QString *error) = 0;
    virtual bool remove(QString *error) = 0;
};
