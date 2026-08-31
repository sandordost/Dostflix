#include "network/NetworkGuardClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <QProcess>
#include <QRegularExpression>

NetworkGuardClient::NetworkGuardClient(QString helperPath)
{
    if (!helperPath.isEmpty()) {
        m_helperPath = std::move(helperPath);
        return;
    }

    const QString developmentHelper = QDir(QCoreApplication::applicationDirPath())
                                          .filePath(QStringLiteral("dostflix-network-helper"));
    m_helperPath = QFileInfo::exists(developmentHelper)
                       ? developmentHelper
                       : QStringLiteral("/usr/lib/dostflix/dostflix-network-helper");
}

bool NetworkGuardClient::loadScope(QString *error)
{
    QFile file(QStringLiteral("/proc/self/cgroup"));
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("Unable to inspect the Dostflix cgroup");
        return false;
    }
    const QString data = QString::fromUtf8(file.readAll());
    for (const QString &line : data.split(QLatin1Char('\n'))) {
        if (!line.startsWith(QStringLiteral("0::"))) continue;
        const QStringList components = line.mid(3).split(QLatin1Char('/'), Qt::SkipEmptyParts);
        if (components.isEmpty()) continue;
        const QRegularExpressionMatch match = QRegularExpression(
            QStringLiteral("^dostflix-([a-f0-9]{32})\\.scope$")).match(components.constLast());
        if (!match.hasMatch()) continue;
        m_session = match.captured(1);
        m_scope = components.constLast();
        m_cgroupLevel = static_cast<int>(components.size());
        return true;
    }
    if (error) *error = QStringLiteral("Dostflix could not enter its protected systemd scope");
    return false;
}

bool NetworkGuardClient::runHelper(const QStringList &arguments, QString *error)
{
    QProcess process;
    process.start(QStringLiteral("/usr/bin/pkexec"), QStringList{m_helperPath} + arguments);
    if (!process.waitForFinished(60'000) || process.exitCode() != 0) {
        if (error) {
            const QString detail = QString::fromUtf8(process.readAllStandardError()).trimmed();
            *error = detail.isEmpty() ? QStringLiteral("Network protection was not authorized") : detail;
        }
        return false;
    }
    return true;
}

bool NetworkGuardClient::install(const VpnTransport &transport, const QString &interfaceName,
                                 const QString &phase, QString *error)
{
    if (m_session.isEmpty() && !loadScope(error)) return false;
    if (m_endpoint.isNull()) {
        const QHostInfo info = QHostInfo::fromName(transport.host);
        if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
            if (error) *error = QStringLiteral("Unable to resolve the VPN endpoint");
            return false;
        }
        for (const QHostAddress &address : info.addresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                m_endpoint = address;
                break;
            }
        }
        if (m_endpoint.isNull()) m_endpoint = info.addresses().first();
    }
    return runHelper({QStringLiteral("install"), QString::number(QCoreApplication::applicationPid()),
                      m_session, m_scope, QString::number(m_cgroupLevel), m_endpoint.toString(),
                      transport.tcp ? QStringLiteral("tcp") : QStringLiteral("udp"),
                      QString::number(transport.port), interfaceName, phase}, error);
}

bool NetworkGuardClient::installBootstrap(const VpnTransport &transport, QString *error)
{
    m_endpoint.clear();
    return install(transport, {}, QStringLiteral("bootstrap"), error);
}

bool NetworkGuardClient::installProtected(const VpnTransport &transport,
                                          const QString &interfaceName, QString *error)
{
    return install(transport, interfaceName, QStringLiteral("protected"), error);
}

bool NetworkGuardClient::remove(QString *error)
{
    if (m_session.isEmpty()) return true;
    const bool result = runHelper({QStringLiteral("remove"),
                                   QString::number(QCoreApplication::applicationPid()),
                                   m_session, m_scope}, error);
    if (result) {
        m_session.clear();
        m_scope.clear();
        m_endpoint.clear();
        m_cgroupLevel = 0;
    }
    return result;
}
