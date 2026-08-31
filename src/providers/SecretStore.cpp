#include "providers/SecretStore.h"

#include <QProcess>

namespace {
bool runSecretTool(const QStringList &arguments, const QString &input, QString *error)
{
    QProcess process;
    process.start(QStringLiteral("/usr/bin/secret-tool"), arguments);
    if (!process.waitForStarted(5'000)) {
        if (error) *error = QStringLiteral("Desktop secret store is unavailable");
        return false;
    }
    if (!input.isNull()) process.write(input.toUtf8());
    process.closeWriteChannel();
    if (!process.waitForFinished(15'000) || process.exitCode() != 0) {
        if (error) *error = QStringLiteral("Could not update the desktop secret store");
        return false;
    }
    if (error) error->clear();
    return true;
}
}

bool LibSecretStore::store(const QString &providerId, const QString &secret, QString *error)
{
    return runSecretTool({QStringLiteral("store"), QStringLiteral("--label=Dostflix provider API key"),
                          QStringLiteral("application"), QStringLiteral("dostflix"),
                          QStringLiteral("provider"), providerId}, secret, error);
}

QString LibSecretStore::load(const QString &providerId, QString *error)
{
    QProcess process;
    process.start(QStringLiteral("/usr/bin/secret-tool"),
                  {QStringLiteral("lookup"), QStringLiteral("application"),
                   QStringLiteral("dostflix"), QStringLiteral("provider"), providerId});
    if (!process.waitForStarted(5'000) || !process.waitForFinished(15'000)) {
        if (error) *error = QStringLiteral("Desktop secret store is unavailable");
        return {};
    }
    if (process.exitCode() != 0) {
        if (error) error->clear();
        return {};
    }
    if (error) error->clear();
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

bool LibSecretStore::remove(const QString &providerId, QString *error)
{
    return runSecretTool({QStringLiteral("clear"), QStringLiteral("application"),
                          QStringLiteral("dostflix"), QStringLiteral("provider"), providerId},
                         QString(), error);
}
