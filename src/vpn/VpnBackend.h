#pragma once

#include <QList>
#include <QString>

struct VpnProfile
{
    QString uuid;
    QString name;

    friend bool operator==(const VpnProfile &, const VpnProfile &) = default;
};

struct VpnTransport
{
    QString host;
    quint16 port = 0;
    bool tcp = false;
};

enum class VpnConnectionState
{
    Inactive,
    Activating,
    Activated,
    Deactivating,
    Failed,
};

class VpnBackend
{
public:
    virtual ~VpnBackend() = default;

    [[nodiscard]] virtual QList<VpnProfile> profiles(QString *error) = 0;
    [[nodiscard]] virtual QString importOpenVpn(const QString &filePath, QString *error) = 0;
    [[nodiscard]] virtual VpnTransport transport(const QString &uuid, QString *error) = 0;
    [[nodiscard]] virtual QString tunnelInterface(const QString &uuid, QString *error) = 0;
    [[nodiscard]] virtual bool routeUsesInterface(const QString &interfaceName,
                                                  QString *error) = 0;
    [[nodiscard]] virtual bool dnsUsesInterface(const QString &interfaceName,
                                                QString *error) = 0;
    [[nodiscard]] virtual VpnConnectionState connectionState(const QString &uuid,
                                                              QString *activePath,
                                                              QString *error) = 0;
    virtual bool activate(const QString &uuid, QString *activePath, QString *error) = 0;
    virtual bool deactivate(const QString &activePath, QString *error) = 0;
};
