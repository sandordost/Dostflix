#pragma once

#include <QSettings>

class AppSettings final
{
public:
    explicit AppSettings(const QString &fileName);

    [[nodiscard]] QString libraryDirectory() const;
    void setLibraryDirectory(const QString &value);
    [[nodiscard]] QString vpnConnectionUuid() const;
    void setVpnConnectionUuid(const QString &value);

private:
    mutable QSettings m_settings;
};
