#include "app/AppSettings.h"

AppSettings::AppSettings(const QString &fileName)
    : m_settings(fileName, QSettings::IniFormat)
{
}

QString AppSettings::libraryDirectory() const
{
    return m_settings.value(QStringLiteral("library/directory")).toString();
}

void AppSettings::setLibraryDirectory(const QString &value)
{
    m_settings.setValue(QStringLiteral("library/directory"), value);
}

QString AppSettings::vpnConnectionUuid() const
{
    return m_settings.value(QStringLiteral("vpn/connectionUuid")).toString();
}

void AppSettings::setVpnConnectionUuid(const QString &value)
{
    m_settings.setValue(QStringLiteral("vpn/connectionUuid"), value);
}
