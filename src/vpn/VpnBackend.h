#pragma once

#include <QList>
#include <QString>

struct VpnProfile
{
    QString uuid;
    QString name;

    friend bool operator==(const VpnProfile &, const VpnProfile &) = default;
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
    [[nodiscard]] virtual VpnConnectionState connectionState(const QString &uuid,
                                                              QString *activePath,
                                                              QString *error) = 0;
    virtual bool activate(const QString &uuid, QString *activePath, QString *error) = 0;
    virtual bool deactivate(const QString &activePath, QString *error) = 0;
};
