#include "network/NetworkGuardRules.h"

#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QTextStream>
#include <unistd.h>

namespace {
bool digitsOnly(const QString &value)
{
    for (const QChar character : value) {
        if (!character.isDigit()) return false;
    }
    return !value.isEmpty();
}

int fail(const QString &message)
{
    QTextStream(stderr) << message << Qt::endl;
    return EXIT_FAILURE;
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (geteuid() != 0) return fail(QStringLiteral("Network guard helper must run as root"));
    if (args.size() < 5) return fail(QStringLiteral("Invalid network guard request"));

    const QString action = args.at(1);
    const QString callerPid = args.at(2);
    const QString session = args.at(3);
    const QString scope = args.at(4);
    if ((action == QStringLiteral("install") && args.size() != 11)
        || (action == QStringLiteral("remove") && args.size() != 5))
        return fail(QStringLiteral("Invalid network guard request"));
    if (action != QStringLiteral("install") && action != QStringLiteral("remove"))
        return fail(QStringLiteral("Invalid network guard action"));
    if (!digitsOnly(callerPid)) return fail(QStringLiteral("Invalid caller process"));

    QFile status(QStringLiteral("/proc/%1/status").arg(callerPid));
    QFile cgroup(QStringLiteral("/proc/%1/cgroup").arg(callerPid));
    if (!status.open(QIODevice::ReadOnly) || !cgroup.open(QIODevice::ReadOnly))
        return fail(QStringLiteral("Calling process no longer exists"));
    const QByteArray expectedUid = qgetenv("PKEXEC_UID");
    if (expectedUid.isEmpty() || !status.readAll().contains("Uid:\t" + expectedUid + "\t"))
        return fail(QStringLiteral("Calling process owner mismatch"));
    QString cgroupPath;
    for (const QString &line : QString::fromUtf8(cgroup.readAll()).split(QLatin1Char('\n'))) {
        if (line.startsWith(QStringLiteral("0::/")) && line.endsWith(QStringLiteral("/") + scope)) {
            cgroupPath = line.mid(4);
            break;
        }
    }
    if (cgroupPath.isEmpty())
        return fail(QStringLiteral("Calling process is outside the requested scope"));

    if (action == QStringLiteral("remove")) {
        const int actualLevel = static_cast<int>(cgroupPath.count(QLatin1Char('/')) + 1);
        NetworkGuardRequest identity{session, scope, cgroupPath, actualLevel,
            QHostAddress(QStringLiteral("127.0.0.1")), GuardTransport::Tcp, 1, {},
            GuardPhase::Bootstrap};
        QString validationError;
        if (!NetworkGuardRules::validate(identity, &validationError)) return fail(validationError);
        QProcess remove;
        remove.start(QStringLiteral("/usr/bin/nft"),
                     {QStringLiteral("delete"), QStringLiteral("table"),
                      QStringLiteral("inet"), NetworkGuardRules::tableName(session)});
        if (!remove.waitForFinished(10'000) || remove.exitCode() != 0)
            return fail(QStringLiteral("Unable to remove Dostflix network guard"));
        return EXIT_SUCCESS;
    }

    const QString levelText = args.at(5);
    const QString endpoint = args.at(6);
    const QString protocol = args.at(7);
    const QString portText = args.at(8);
    const QString interfaceName = args.at(9);
    const QString phase = args.at(10);
    if (!digitsOnly(levelText) || !digitsOnly(portText))
        return fail(QStringLiteral("Invalid numeric field"));
    if (levelText.toInt() != cgroupPath.count(QLatin1Char('/')) + 1)
        return fail(QStringLiteral("Cgroup level mismatch"));

    NetworkGuardRequest request{session, scope, cgroupPath, levelText.toInt(), QHostAddress(endpoint),
        protocol == QStringLiteral("tcp") ? GuardTransport::Tcp : GuardTransport::Udp,
        static_cast<quint16>(portText.toUInt()), interfaceName,
        phase == QStringLiteral("protected") ? GuardPhase::Protected : GuardPhase::Bootstrap};
    if ((protocol != QStringLiteral("tcp") && protocol != QStringLiteral("udp"))
        || (phase != QStringLiteral("bootstrap") && phase != QStringLiteral("protected")))
        return fail(QStringLiteral("Invalid guard mode"));
    QString error;
    const QString rules = NetworkGuardRules::build(request, &error);
    if (rules.isEmpty()) return fail(error);

    QProcess nft;
    nft.start(QStringLiteral("/usr/bin/nft"), {QStringLiteral("-f"), QStringLiteral("-")});
    if (!nft.waitForStarted()) return fail(QStringLiteral("Unable to start nftables"));
    nft.write(rules.toUtf8());
    nft.closeWriteChannel();
    if (!nft.waitForFinished(10'000) || nft.exitCode() != 0) {
        const QString detail = QString::fromUtf8(nft.readAllStandardError()).trimmed();
        return fail(detail.isEmpty() ? QStringLiteral("nftables rejected the guarded ruleset")
                                     : detail);
    }
    return EXIT_SUCCESS;
}
