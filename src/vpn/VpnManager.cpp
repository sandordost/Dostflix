#include "vpn/VpnManager.h"

#include "app/AppSettings.h"
#include "vpn/VpnBackend.h"

#include <QFileInfo>

VpnManager::VpnManager(AppSettings &settings, VpnBackend &backend, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_backend(backend)
    , m_selectedUuid(settings.vpnConnectionUuid())
{
    m_pollTimer.setInterval(1'000);
    connect(&m_pollTimer, &QTimer::timeout, this, &VpnManager::updateConnectionState);
}

VpnProfileModel *VpnManager::profileModel() { return &m_profiles; }
QString VpnManager::selectedProfileUuid() const { return m_selectedUuid; }
QString VpnManager::errorMessage() const { return m_error; }
bool VpnManager::busy() const { return m_state == State::Connecting || m_state == State::Disconnecting; }
bool VpnManager::connected() const { return m_state == State::Connected; }
bool VpnManager::ownsConnection() const { return m_ownsConnection; }

QString VpnManager::stateLabel() const
{
    switch (m_state) {
    case State::NotConfigured: return tr("VPN not configured");
    case State::Disconnected: return tr("VPN disconnected");
    case State::Connecting: return tr("VPN connecting…");
    case State::Connected: return tr("VPN connected");
    case State::Disconnecting: return tr("VPN disconnecting…");
    case State::Error: return tr("VPN error");
    }
    return {};
}

void VpnManager::setState(State state, QString error)
{
    if (m_state == state && m_error == error) {
        return;
    }
    m_state = state;
    m_error = std::move(error);
    emit stateChanged();
}

void VpnManager::refreshProfiles()
{
    QString error;
    const QList<VpnProfile> result = m_backend.profiles(&error);
    if (!error.isEmpty()) {
        setState(State::Error, error);
        return;
    }
    m_profiles.replaceProfiles(result);
    bool selectionExists = false;
    for (const VpnProfile &profile : result) {
        selectionExists |= profile.uuid == m_selectedUuid;
    }
    if (!selectionExists && !m_selectedUuid.isEmpty()) {
        selectProfile({});
    }
    if (m_selectedUuid.isEmpty()) {
        setState(State::NotConfigured);
    }
}

void VpnManager::selectProfile(const QString &uuid)
{
    if (m_selectedUuid == uuid) {
        return;
    }
    if (m_ownsConnection) {
        disconnectOwned();
        if (m_ownsConnection) {
            return;
        }
    }
    m_selectedUuid = uuid;
    m_settings.setVpnConnectionUuid(uuid);
    emit selectedProfileChanged();
    setState(uuid.isEmpty() ? State::NotConfigured : State::Disconnected);
}

void VpnManager::importProfile(const QUrl &fileUrl)
{
    if (!fileUrl.isLocalFile()) {
        setState(State::Error, tr("Choose a local .ovpn file"));
        return;
    }
    QString error;
    const QString uuid = m_backend.importOpenVpn(fileUrl.toLocalFile(), &error);
    if (uuid.isEmpty()) {
        setState(State::Error, error);
        return;
    }
    refreshProfiles();
    selectProfile(uuid);
}

void VpnManager::connectSelected()
{
    if (m_selectedUuid.isEmpty()) {
        setState(State::NotConfigured);
        return;
    }
    QString error;
    QString existingPath;
    const VpnConnectionState current = m_backend.connectionState(m_selectedUuid, &existingPath, &error);
    if (current == VpnConnectionState::Activated || current == VpnConnectionState::Activating) {
        m_activePath = existingPath;
        m_ownsConnection = false;
        setState(current == VpnConnectionState::Activated ? State::Connected : State::Connecting);
        m_pollTimer.start();
        return;
    }
    if (current == VpnConnectionState::Failed) {
        setState(State::Error, error);
        return;
    }
    if (!m_backend.activate(m_selectedUuid, &m_activePath, &error)) {
        setState(State::Error, error);
        return;
    }
    m_ownsConnection = true;
    setState(State::Connecting);
    m_pollTimer.start();
}

void VpnManager::disconnectOwned()
{
    if (!m_ownsConnection) {
        return;
    }
    setState(State::Disconnecting);
    QString error;
    if (!m_backend.deactivate(m_activePath, &error)) {
        setState(State::Error, error);
        return;
    }
    m_pollTimer.stop();
    m_activePath.clear();
    m_ownsConnection = false;
    setState(State::Disconnected);
}

void VpnManager::updateConnectionState()
{
    QString error;
    QString activePath;
    const VpnConnectionState current = m_backend.connectionState(m_selectedUuid, &activePath, &error);
    if (current == VpnConnectionState::Failed) {
        setState(State::Error, error);
        return;
    }
    if (!activePath.isEmpty()) {
        m_activePath = activePath;
    }
    if (current == VpnConnectionState::Activated) {
        setState(State::Connected);
    } else if (current == VpnConnectionState::Activating) {
        setState(State::Connecting);
    } else if (current == VpnConnectionState::Deactivating) {
        setState(State::Disconnecting);
    } else {
        m_pollTimer.stop();
        m_activePath.clear();
        m_ownsConnection = false;
        setState(State::Disconnected);
    }
}

void VpnManager::start()
{
    refreshProfiles();
    if (!m_selectedUuid.isEmpty()) {
        connectSelected();
    }
}

void VpnManager::shutdown()
{
    m_pollTimer.stop();
    disconnectOwned();
}
