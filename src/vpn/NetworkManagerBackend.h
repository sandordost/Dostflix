#pragma once

#include "vpn/VpnBackend.h"

#include <QStringList>

class NetworkManagerBackend final : public VpnBackend
{
public:
    [[nodiscard]] QList<VpnProfile> profiles(QString *error) override;
    [[nodiscard]] QString importOpenVpn(const QString &filePath, QString *error) override;
    [[nodiscard]] VpnTransport transport(const QString &uuid, QString *error) override;
    [[nodiscard]] QString tunnelInterface(const QString &uuid, QString *error) override;
    [[nodiscard]] bool routeUsesInterface(const QString &interfaceName, QString *error) override;
    [[nodiscard]] bool dnsUsesInterface(const QString &interfaceName, QString *error) override;
    [[nodiscard]] VpnConnectionState connectionState(const QString &uuid,
                                                      QString *activePath,
                                                      QString *error) override;
    bool activate(const QString &uuid, QString *activePath, QString *error) override;
    bool deactivate(const QString &activePath, QString *error) override;

    [[nodiscard]] static QList<VpnProfile> parseNmcliProfiles(const QByteArray &output);
    [[nodiscard]] static VpnTransport parseOpenVpnData(const QString &data, QString *error);
    [[nodiscard]] static QString parseRouteDevice(const QByteArray &output);

private:
    [[nodiscard]] static QStringList splitEscapedFields(const QByteArray &line);
};
