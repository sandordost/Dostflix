#include "vpn/NetworkManagerBackend.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QFileInfo>
#include <QProcess>
#include <QSet>

namespace {
constexpr auto service = "org.freedesktop.NetworkManager";
constexpr auto managerPath = "/org/freedesktop/NetworkManager";
constexpr auto managerInterface = "org.freedesktop.NetworkManager";
constexpr auto settingsPath = "/org/freedesktop/NetworkManager/Settings";
constexpr auto settingsInterface = "org.freedesktop.NetworkManager.Settings";
constexpr auto activeInterface = "org.freedesktop.NetworkManager.Connection.Active";

QString processError(QProcess &process)
{
    const QString standardError = QString::fromUtf8(process.readAllStandardError()).trimmed();
    return standardError.isEmpty() ? QStringLiteral("NetworkManager command failed") : standardError;
}
}

QStringList NetworkManagerBackend::splitEscapedFields(const QByteArray &line)
{
    QStringList fields;
    QString current;
    bool escaped = false;
    for (const char byte : line) {
        const QChar character = QLatin1Char(byte);
        if (escaped) {
            current += character;
            escaped = false;
        } else if (character == QLatin1Char('\\')) {
            escaped = true;
        } else if (character == QLatin1Char(':')) {
            fields.push_back(current);
            current.clear();
        } else {
            current += character;
        }
    }
    if (escaped) {
        current += QLatin1Char('\\');
    }
    fields.push_back(current);
    return fields;
}

QList<VpnProfile> NetworkManagerBackend::parseNmcliProfiles(const QByteArray &output)
{
    QList<VpnProfile> result;
    for (const QByteArray &line : output.split('\n')) {
        if (line.trimmed().isEmpty()) {
            continue;
        }
        const QStringList fields = splitEscapedFields(line);
        if (fields.size() == 3 && fields.at(2) == QStringLiteral("vpn")
            && !fields.at(0).isEmpty()) {
            result.push_back({fields.at(0), fields.at(1)});
        }
    }
    return result;
}

QList<VpnProfile> NetworkManagerBackend::profiles(QString *error)
{
    QProcess process;
    process.start(QStringLiteral("nmcli"),
                  {QStringLiteral("--terse"), QStringLiteral("--escape"),
                   QStringLiteral("yes"), QStringLiteral("--fields"),
                   QStringLiteral("UUID,NAME,TYPE"), QStringLiteral("connection"),
                   QStringLiteral("show")});
    if (!process.waitForFinished(10'000) || process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != 0) {
        if (error != nullptr) {
            *error = processError(process);
        }
        return {};
    }
    return parseNmcliProfiles(process.readAllStandardOutput());
}

QString NetworkManagerBackend::importOpenVpn(const QString &filePath, QString *error)
{
    const QFileInfo source(filePath);
    if (!source.isFile() || source.suffix().compare(QStringLiteral("ovpn"), Qt::CaseInsensitive) != 0) {
        if (error != nullptr) {
            *error = QStringLiteral("Choose an existing .ovpn file");
        }
        return {};
    }

    QString beforeError;
    const QList<VpnProfile> before = profiles(&beforeError);
    QSet<QString> known;
    for (const VpnProfile &profile : before) {
        known.insert(profile.uuid);
    }

    QProcess process;
    process.start(QStringLiteral("nmcli"),
                  {QStringLiteral("--wait"), QStringLiteral("20"),
                   QStringLiteral("connection"), QStringLiteral("import"),
                   QStringLiteral("type"), QStringLiteral("openvpn"),
                   QStringLiteral("file"), source.absoluteFilePath()});
    if (!process.waitForFinished(25'000) || process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != 0) {
        if (error != nullptr) {
            *error = processError(process);
        }
        return {};
    }

    QString afterError;
    const QList<VpnProfile> after = profiles(&afterError);
    for (const VpnProfile &profile : after) {
        if (!known.contains(profile.uuid)) {
            return profile.uuid;
        }
    }
    if (error != nullptr) {
        *error = afterError.isEmpty() ? QStringLiteral("Imported profile was not found") : afterError;
    }
    return {};
}

VpnConnectionState NetworkManagerBackend::connectionState(const QString &uuid,
                                                           QString *activePath,
                                                           QString *error)
{
    QDBusInterface properties(service, managerPath,
                              QStringLiteral("org.freedesktop.DBus.Properties"),
                              QDBusConnection::systemBus());
    const QDBusReply<QVariant> activeReply = properties.call(
        QStringLiteral("Get"), QString::fromLatin1(managerInterface),
        QStringLiteral("ActiveConnections"));
    if (!activeReply.isValid()) {
        if (error != nullptr) {
            *error = activeReply.error().message();
        }
        return VpnConnectionState::Failed;
    }

    const auto paths = qdbus_cast<QList<QDBusObjectPath>>(activeReply.value());
    for (const QDBusObjectPath &path : paths) {
        QDBusInterface activeProperties(service, path.path(),
                                        QStringLiteral("org.freedesktop.DBus.Properties"),
                                        QDBusConnection::systemBus());
        const QDBusReply<QVariant> uuidReply = activeProperties.call(
            QStringLiteral("Get"), QString::fromLatin1(activeInterface), QStringLiteral("Uuid"));
        if (!uuidReply.isValid() || uuidReply.value().toString() != uuid) {
            continue;
        }
        if (activePath != nullptr) {
            *activePath = path.path();
        }
        const QDBusReply<QVariant> stateReply = activeProperties.call(
            QStringLiteral("Get"), QString::fromLatin1(activeInterface), QStringLiteral("State"));
        if (!stateReply.isValid()) {
            if (error != nullptr) {
                *error = stateReply.error().message();
            }
            return VpnConnectionState::Failed;
        }
        switch (stateReply.value().toUInt()) {
        case 1: return VpnConnectionState::Activating;
        case 2: return VpnConnectionState::Activated;
        case 3: return VpnConnectionState::Deactivating;
        default: return VpnConnectionState::Inactive;
        }
    }
    return VpnConnectionState::Inactive;
}

bool NetworkManagerBackend::activate(const QString &uuid, QString *activePath, QString *error)
{
    QDBusInterface settings(service, settingsPath, settingsInterface,
                            QDBusConnection::systemBus());
    const QDBusReply<QDBusObjectPath> profileReply = settings.call(
        QStringLiteral("GetConnectionByUuid"), uuid);
    if (!profileReply.isValid()) {
        if (error != nullptr) {
            *error = profileReply.error().message();
        }
        return false;
    }

    QDBusInterface manager(service, managerPath, managerInterface,
                           QDBusConnection::systemBus());
    const QDBusReply<QDBusObjectPath> activeReply = manager.call(
        QStringLiteral("ActivateConnection"), QVariant::fromValue(profileReply.value()),
        QVariant::fromValue(QDBusObjectPath(QStringLiteral("/"))),
        QVariant::fromValue(QDBusObjectPath(QStringLiteral("/"))));
    if (!activeReply.isValid()) {
        if (error != nullptr) {
            *error = activeReply.error().message();
        }
        return false;
    }
    if (activePath != nullptr) {
        *activePath = activeReply.value().path();
    }
    return true;
}

bool NetworkManagerBackend::deactivate(const QString &activePath, QString *error)
{
    if (activePath.isEmpty()) {
        return true;
    }
    QDBusInterface manager(service, managerPath, managerInterface,
                           QDBusConnection::systemBus());
    const QDBusReply<void> reply = manager.call(
        QStringLiteral("DeactivateConnection"),
        QVariant::fromValue(QDBusObjectPath(activePath)));
    if (!reply.isValid()) {
        if (error != nullptr) {
            *error = reply.error().message();
        }
        return false;
    }
    return true;
}
