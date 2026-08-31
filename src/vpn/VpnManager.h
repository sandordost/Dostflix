#pragma once

#include "vpn/VpnProfileModel.h"

#include <QObject>
#include <QTimer>
#include <QUrl>

class AppSettings;
class VpnBackend;

class VpnManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(VpnProfileModel *profileModel READ profileModel CONSTANT)
    Q_PROPERTY(QString selectedProfileUuid READ selectedProfileUuid NOTIFY selectedProfileChanged)
    Q_PROPERTY(QString stateLabel READ stateLabel NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY stateChanged)
    Q_PROPERTY(bool ownsConnection READ ownsConnection NOTIFY stateChanged)

public:
    enum class State { NotConfigured, Disconnected, Connecting, Connected, Disconnecting, Error };
    Q_ENUM(State)

    VpnManager(AppSettings &settings, VpnBackend &backend, QObject *parent = nullptr);

    [[nodiscard]] VpnProfileModel *profileModel();
    [[nodiscard]] QString selectedProfileUuid() const;
    [[nodiscard]] QString stateLabel() const;
    [[nodiscard]] QString errorMessage() const;
    [[nodiscard]] bool busy() const;
    [[nodiscard]] bool connected() const;
    [[nodiscard]] bool ownsConnection() const;

    Q_INVOKABLE void refreshProfiles();
    Q_INVOKABLE void selectProfile(const QString &uuid);
    Q_INVOKABLE void importProfile(const QUrl &fileUrl);
    Q_INVOKABLE void connectSelected();
    Q_INVOKABLE void disconnectOwned();
    void start();
    void shutdown();

signals:
    void selectedProfileChanged();
    void stateChanged();

private:
    void updateConnectionState();
    void setState(State state, QString error = {});

    AppSettings &m_settings;
    VpnBackend &m_backend;
    VpnProfileModel m_profiles;
    QTimer m_pollTimer;
    QString m_selectedUuid;
    QString m_activePath;
    QString m_error;
    State m_state = State::NotConfigured;
    bool m_ownsConnection = false;
};
