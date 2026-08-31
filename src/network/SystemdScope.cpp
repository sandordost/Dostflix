#include "network/SystemdScope.h"

#include <QCoreApplication>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QThread>
#include <QUuid>

namespace {
const QRegularExpression scopePattern(
    QStringLiteral("(?:^|/)dostflix-[a-f0-9]{32}\\.scope$"));

bool isInDostflixScope()
{
    QFile file(QStringLiteral("/proc/self/cgroup"));
    if (!file.open(QIODevice::ReadOnly)) return false;
    for (const QString &line : QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'))) {
        if (line.startsWith(QStringLiteral("0::/"))
            && scopePattern.match(line.mid(4)).hasMatch()) return true;
    }
    return false;
}
}

bool SystemdScope::enter(QString *error)
{
    if (isInDostflixScope()) return true;

    QString session = QUuid::createUuid().toString(QUuid::WithoutBraces);
    session.remove(QLatin1Char('-'));
    const QString scope = QStringLiteral("dostflix-%1.scope").arg(session);
    QProcess busctl;
    busctl.start(QStringLiteral("/usr/bin/busctl"), {
        QStringLiteral("--user"), QStringLiteral("call"),
        QStringLiteral("org.freedesktop.systemd1"),
        QStringLiteral("/org/freedesktop/systemd1"),
        QStringLiteral("org.freedesktop.systemd1.Manager"),
        QStringLiteral("StartTransientUnit"),
        QStringLiteral("ssa(sv)a(sa(sv))"), scope, QStringLiteral("fail"),
        QStringLiteral("3"), QStringLiteral("Description"), QStringLiteral("s"),
        QStringLiteral("Dostflix protected application scope"),
        QStringLiteral("CollectMode"), QStringLiteral("s"),
        QStringLiteral("inactive-or-failed"),
        QStringLiteral("PIDs"), QStringLiteral("au"), QStringLiteral("1"),
        QString::number(QCoreApplication::applicationPid()), QStringLiteral("0")
    });
    if (!busctl.waitForFinished(5'000) || busctl.exitCode() != 0) {
        if (error) *error = QStringLiteral("Unable to create the protected systemd scope");
        return false;
    }
    for (int attempt = 0; attempt < 50; ++attempt) {
        if (isInDostflixScope()) return true;
        QThread::msleep(20);
    }
    if (error) *error = QStringLiteral("systemd did not attach Dostflix to its protected scope");
    return false;
}
