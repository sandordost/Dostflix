#pragma once

#include "providers/ProviderConfig.h"

#include <QSettings>

class AppSettings final
{
public:
    explicit AppSettings(const QString &fileName);

    [[nodiscard]] QString libraryDirectory() const;
    void setLibraryDirectory(const QString &value);
    [[nodiscard]] QString vpnConnectionUuid() const;
    void setVpnConnectionUuid(const QString &value);
    [[nodiscard]] QString ownedVpnConnectionUuid() const;
    [[nodiscard]] qint64 vpnOwnerPid() const;
    void setVpnOwnership(const QString &uuid, qint64 ownerPid);
    void clearVpnOwnership();
    [[nodiscard]] QList<ProviderConfig> providers() const;
    void setProviders(const QList<ProviderConfig> &providers);

private:
    mutable QSettings m_settings;
};
