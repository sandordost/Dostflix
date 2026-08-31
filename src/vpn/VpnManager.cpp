#include "vpn/VpnManager.h"

#include "app/AppSettings.h"
#include "network/NetworkGuardBackend.h"
#include "vpn/VpnBackend.h"

#include <QFileInfo>

VpnManager::VpnManager(AppSettings &settings, VpnBackend &backend,
                       NetworkGuardBackend *guard, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_backend(backend)
    , m_guard(guard)
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
bool VpnManager::networkReady() const { return connected() && m_guardProtected; }

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
    } else if (!removeGuard()) {
        return;
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
    if (!installBootstrapGuard()) return;
    QString error;
    QString existingPath;
    const VpnConnectionState current = m_backend.connectionState(m_selectedUuid, &existingPath, &error);
    if (current == VpnConnectionState::Activated || current == VpnConnectionState::Activating) {
        m_activePath = existingPath;
        m_ownsConnection = false;
        setState(current == VpnConnectionState::Activated ? State::Connected : State::Connecting);
        if (current == VpnConnectionState::Activated && !installProtectedGuard()) return;
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
    if (!removeGuard()) return;
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
        if (!installProtectedGuard()) return;
        setState(State::Connected);
    } else if (current == VpnConnectionState::Activating) {
        setState(State::Connecting);
    } else if (current == VpnConnectionState::Deactivating) {
        setState(State::Disconnecting);
    } else {
        m_pollTimer.stop();
        m_activePath.clear();
        m_ownsConnection = false;
        m_guardProtected = false;
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
    removeGuard();
    disconnectOwned();
}

bool VpnManager::installBootstrapGuard()
{
    if (m_guard == nullptr) return true;
    if (m_guardInstalled && !removeGuard()) return false;
    QString error;
    m_transport = m_backend.transport(m_selectedUuid, &error);
    if (!error.isEmpty() || m_transport.host.isEmpty()
        || !m_guard->installBootstrap(m_transport, &error)) {
        setState(State::Error, error.isEmpty() ? tr("VPN transport is invalid") : error);
        return false;
    }
    m_guardInstalled = true;
    m_guardProtected = false;
    emit stateChanged();
    return true;
}

bool VpnManager::installProtectedGuard()
{
    if (m_guard == nullptr || m_guardProtected) return true;
    QString error;
    const QString interfaceName = m_backend.tunnelInterface(m_selectedUuid, &error);
    if (interfaceName.isEmpty() || !m_backend.routeUsesInterface(interfaceName, &error)
        || !m_backend.dnsUsesInterface(interfaceName, &error)
        || !m_guard->installProtected(m_transport, interfaceName, &error)) {
        setState(State::Error, error.isEmpty() ? tr("VPN tunnel interface is unavailable") : error);
        return false;
    }
    m_guardProtected = true;
    emit stateChanged();
    return true;
}

bool VpnManager::removeGuard()
{
    if (m_guard == nullptr || !m_guardInstalled) return true;
    QString error;
    if (!m_guard->remove(&error)) {
        setState(State::Error, error);
        return false;
    }
    m_guardInstalled = false;
    m_guardProtected = false;
    emit stateChanged();
    return true;
}
