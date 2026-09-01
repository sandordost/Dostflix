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

QString AppSettings::ownedVpnConnectionUuid() const
{
    return m_settings.value(QStringLiteral("vpn/ownedConnectionUuid")).toString();
}

qint64 AppSettings::vpnOwnerPid() const
{
    return m_settings.value(QStringLiteral("vpn/ownerPid"), 0).toLongLong();
}

void AppSettings::setVpnOwnership(const QString &uuid, qint64 ownerPid)
{
    m_settings.setValue(QStringLiteral("vpn/ownedConnectionUuid"), uuid);
    m_settings.setValue(QStringLiteral("vpn/ownerPid"), ownerPid);
    m_settings.sync();
}

void AppSettings::clearVpnOwnership()
{
    m_settings.remove(QStringLiteral("vpn/ownedConnectionUuid"));
    m_settings.remove(QStringLiteral("vpn/ownerPid"));
    m_settings.sync();
}

QList<ProviderConfig> AppSettings::providers() const
{
    QList<ProviderConfig> result;
    const int count = m_settings.beginReadArray(QStringLiteral("providers"));
    result.reserve(count);
    for (int index = 0; index < count; ++index) {
        m_settings.setArrayIndex(index);
        ProviderConfig provider;
        provider.id = m_settings.value(QStringLiteral("id")).toString();
        provider.name = m_settings.value(QStringLiteral("name")).toString();
        provider.kind = m_settings.value(QStringLiteral("kind")).toString()
                                == QStringLiteral("prowlarr")
                            ? ProviderKind::Prowlarr : ProviderKind::Torznab;
        provider.endpoint = m_settings.value(QStringLiteral("endpoint")).toUrl();
        provider.enabled = m_settings.value(QStringLiteral("enabled"), true).toBool();
        result.push_back(provider);
    }
    m_settings.endArray();
    return result;
}

void AppSettings::setProviders(const QList<ProviderConfig> &providers)
{
    m_settings.beginWriteArray(QStringLiteral("providers"), static_cast<int>(providers.size()));
    for (qsizetype index = 0; index < providers.size(); ++index) {
        m_settings.setArrayIndex(static_cast<int>(index));
        const ProviderConfig &provider = providers.at(index);
        m_settings.setValue(QStringLiteral("id"), provider.id);
        m_settings.setValue(QStringLiteral("name"), provider.name);
        m_settings.setValue(QStringLiteral("kind"), provider.kind == ProviderKind::Prowlarr
                                                       ? QStringLiteral("prowlarr")
                                                       : QStringLiteral("torznab"));
        m_settings.setValue(QStringLiteral("endpoint"), provider.endpoint);
        m_settings.setValue(QStringLiteral("enabled"), provider.enabled);
    }
    m_settings.endArray();
}
