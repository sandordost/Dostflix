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
    if (args.size() != 10) return fail(QStringLiteral("Invalid network guard request"));

    const QString callerPid = args.at(1);
    const QString session = args.at(2);
    const QString scope = args.at(3);
    const QString levelText = args.at(4);
    const QString endpoint = args.at(5);
    const QString protocol = args.at(6);
    const QString portText = args.at(7);
    const QString interfaceName = args.at(8);
    const QString phase = args.at(9);
    if (!digitsOnly(callerPid) || !digitsOnly(levelText) || !digitsOnly(portText))
        return fail(QStringLiteral("Invalid numeric field"));

    QFile status(QStringLiteral("/proc/%1/status").arg(callerPid));
    QFile cgroup(QStringLiteral("/proc/%1/cgroup").arg(callerPid));
    if (!status.open(QIODevice::ReadOnly) || !cgroup.open(QIODevice::ReadOnly))
        return fail(QStringLiteral("Calling process no longer exists"));
    const QByteArray expectedUid = qgetenv("PKEXEC_UID");
    if (expectedUid.isEmpty() || !status.readAll().contains("Uid:\t" + expectedUid + "\t"))
        return fail(QStringLiteral("Calling process owner mismatch"));
    if (!cgroup.readAll().contains((QStringLiteral("/") + scope).toUtf8()))
        return fail(QStringLiteral("Calling process is outside the requested scope"));

    NetworkGuardRequest request{session, scope, levelText.toInt(), QHostAddress(endpoint),
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
    if (!nft.waitForFinished(10'000) || nft.exitCode() != 0)
        return fail(QStringLiteral("nftables rejected the guarded ruleset"));
    return EXIT_SUCCESS;
}
