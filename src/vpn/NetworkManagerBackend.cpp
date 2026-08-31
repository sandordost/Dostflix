#include "vpn/NetworkManagerBackend.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QFileInfo>
#include <QFile>
#include <QHostAddress>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
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

VpnTransport NetworkManagerBackend::parseOpenVpnData(const QString &data, QString *error)
{
    const QRegularExpression remoteExpression(
        QStringLiteral("(?:^|, )remote = ((?:\\\\.|[^,])+)"));
    const QRegularExpressionMatch match = remoteExpression.match(data);
    if (!match.hasMatch()) {
        if (error) *error = QStringLiteral("OpenVPN profile has no transport endpoint");
        return {};
    }
    QString remote = match.captured(1);
    remote.replace(QStringLiteral("\\:"), QStringLiteral(":"));
    QString host = remote;
    quint16 port = 1194;
    const qsizetype separator = remote.lastIndexOf(QLatin1Char(':'));
    bool portOk = false;
    const uint parsedPort = separator > 0 ? remote.mid(separator + 1).toUInt(&portOk) : 0;
    if (portOk && parsedPort <= 65'535) {
        host = remote.left(separator);
        port = static_cast<quint16>(parsedPort);
    }
    if (host.startsWith(QLatin1Char('[')) && host.endsWith(QLatin1Char(']')))
        host = host.mid(1, host.size() - 2);
    const bool tcp = data.contains(QStringLiteral("proto-tcp = yes"))
                     || data.contains(QStringLiteral("proto = tcp"));
    if (host.isEmpty() || port == 0) {
        if (error) *error = QStringLiteral("OpenVPN transport endpoint is invalid");
        return {};
    }
    return {host, port, tcp};
}

VpnTransport NetworkManagerBackend::transport(const QString &uuid, QString *error)
{
    QProcess process;
    process.start(QStringLiteral("nmcli"), {QStringLiteral("--get-values"),
        QStringLiteral("vpn.service-type,vpn.data"), QStringLiteral("connection"),
        QStringLiteral("show"), QStringLiteral("uuid"), uuid});
    if (!process.waitForFinished(10'000) || process.exitCode() != 0) {
        if (error) *error = processError(process);
        return {};
    }
    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    if (!output.startsWith(QStringLiteral("org.freedesktop.NetworkManager.openvpn\n"))) {
        if (error) *error = QStringLiteral("Selected profile is not an OpenVPN connection");
        return {};
    }
    return parseOpenVpnData(output.section(QLatin1Char('\n'), 1), error);
}

QString NetworkManagerBackend::tunnelInterface(const QString &uuid, QString *error)
{
    Q_UNUSED(uuid)
    QProcess process;
    process.start(QStringLiteral("/usr/bin/ip"), {QStringLiteral("-4"), QStringLiteral("route"),
        QStringLiteral("get"), QStringLiteral("1.1.1.1")});
    if (!process.waitForFinished(5'000) || process.exitCode() != 0) {
        if (error) *error = processError(process);
        return {};
    }
    const QString device = parseRouteDevice(process.readAllStandardOutput());
    if (!device.isEmpty()
        && QFileInfo::exists(QStringLiteral("/sys/class/net/%1/tun_flags").arg(device))) {
        return device;
    }
    if (error) *error = QStringLiteral("VPN tunnel interface is not ready");
    return {};
}

QString NetworkManagerBackend::parseRouteDevice(const QByteArray &output)
{
    const QStringList fields = QString::fromUtf8(output).split(
        QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    const qsizetype deviceIndex = fields.indexOf(QStringLiteral("dev"));
    return deviceIndex >= 0 && deviceIndex + 1 < fields.size()
               ? fields.at(deviceIndex + 1) : QString{};
}

QStringList NetworkManagerBackend::fullTunnelArguments(const QString &uuid)
{
    return {QStringLiteral("connection"), QStringLiteral("modify"),
            QStringLiteral("uuid"), uuid,
            QStringLiteral("ipv4.never-default"), QStringLiteral("no"),
            QStringLiteral("ipv6.never-default"), QStringLiteral("no"),
            QStringLiteral("ipv4.ignore-auto-dns"), QStringLiteral("no"),
            QStringLiteral("ipv6.ignore-auto-dns"), QStringLiteral("no"),
            QStringLiteral("ipv4.dns-priority"), QStringLiteral("-50"),
            QStringLiteral("ipv6.dns-priority"), QStringLiteral("-50")};
}

bool NetworkManagerBackend::routeUsesInterface(const QString &interfaceName, QString *error)
{
    QProcess process;
    process.start(QStringLiteral("/usr/bin/ip"), {QStringLiteral("-4"), QStringLiteral("route"),
        QStringLiteral("get"), QStringLiteral("1.1.1.1")});
    if (!process.waitForFinished(5'000) || process.exitCode() != 0) {
        if (error) *error = processError(process);
        return false;
    }
    const QStringList fields = QString::fromUtf8(process.readAllStandardOutput())
                                   .split(QRegularExpression(QStringLiteral("\\s+")),
                                          Qt::SkipEmptyParts);
    const qsizetype deviceIndex = fields.indexOf(QStringLiteral("dev"));
    if (deviceIndex < 0 || deviceIndex + 1 >= fields.size()
        || fields.at(deviceIndex + 1) != interfaceName) {
        if (error) *error = QStringLiteral("Default IPv4 route does not use the VPN interface");
        return false;
    }
    return true;
}

bool NetworkManagerBackend::dnsUsesInterface(const QString &interfaceName, QString *error)
{
    QFile resolvConf(QStringLiteral("/etc/resolv.conf"));
    if (!resolvConf.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("Unable to inspect DNS configuration");
        return false;
    }
    QStringList servers;
    const QRegularExpression nameserverExpression(
        QStringLiteral("^\\s*nameserver\\s+(\\S+)"), QRegularExpression::MultilineOption);
    auto matches = nameserverExpression.globalMatch(QString::fromUtf8(resolvConf.readAll()));
    while (matches.hasNext()) servers.push_back(matches.next().captured(1));
    if (servers.isEmpty()) {
        if (error) *error = QStringLiteral("No DNS servers are configured");
        return false;
    }

    for (const QString &server : servers) {
        const QHostAddress address(server.section(QLatin1Char('%'), 0, 0));
        if (address.isNull()) {
            if (error) *error = QStringLiteral("DNS server address is invalid");
            return false;
        }
        if (address.isLoopback()) {
            QProcess resolved;
            QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
            environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
            resolved.setProcessEnvironment(environment);
            resolved.start(QStringLiteral("/usr/bin/resolvectl"),
                           {QStringLiteral("status"), interfaceName});
            if (!resolved.waitForFinished(5'000) || resolved.exitCode() != 0) {
                if (error) *error = QStringLiteral("Local DNS resolver is not bound to the VPN");
                return false;
            }
            const QString status = QString::fromUtf8(resolved.readAllStandardOutput());
            if (!status.contains(QStringLiteral("DNS Servers:"))
                || (!status.contains(QStringLiteral("DNS Domain: ~."))
                    && !status.contains(QStringLiteral("DefaultRoute setting: yes")))) {
                if (error) *error = QStringLiteral("Local DNS resolver has no VPN default route");
                return false;
            }
            continue;
        }
        QProcess route;
        route.start(QStringLiteral("/usr/bin/ip"),
                    {address.protocol() == QAbstractSocket::IPv6Protocol
                         ? QStringLiteral("-6") : QStringLiteral("-4"),
                     QStringLiteral("route"), QStringLiteral("get"), address.toString()});
        if (!route.waitForFinished(5'000) || route.exitCode() != 0) {
            if (error) *error = QStringLiteral("DNS server route is unavailable");
            return false;
        }
        const QStringList fields = QString::fromUtf8(route.readAllStandardOutput())
                                       .split(QRegularExpression(QStringLiteral("\\s+")),
                                              Qt::SkipEmptyParts);
        const qsizetype deviceIndex = fields.indexOf(QStringLiteral("dev"));
        if (deviceIndex < 0 || deviceIndex + 1 >= fields.size()
            || fields.at(deviceIndex + 1) != interfaceName) {
            if (error) *error = QStringLiteral("DNS server route bypasses the VPN interface");
            return false;
        }
    }
    return true;
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
    QProcess configure;
    configure.start(QStringLiteral("/usr/bin/nmcli"), fullTunnelArguments(uuid));
    if (!configure.waitForFinished(10'000) || configure.exitCode() != 0) {
        if (error) *error = processError(configure);
        return false;
    }

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
