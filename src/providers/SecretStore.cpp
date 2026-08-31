#include "providers/SecretStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace {
QString credentialsPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
        .filePath(QStringLiteral("dostflix/credentials.ini"));
}

bool securePermissions(const QString &path, QString *error)
{
    if (QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        if (error) error->clear();
        return true;
    }
    if (error) *error = QStringLiteral("Could not secure the local credential file");
    return false;
}
}

bool LibSecretStore::store(const QString &providerId, const QString &secret, QString *error)
{
    const QString path = credentialsPath();
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        if (error) *error = QStringLiteral("Could not create the credential directory");
        return false;
    }
    QSettings settings(path, QSettings::IniFormat);
    settings.setValue(QStringLiteral("secrets/%1").arg(providerId), secret);
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        if (error) *error = QStringLiteral("Could not update the local credential file");
        return false;
    }
    return securePermissions(path, error);
}

QString LibSecretStore::load(const QString &providerId, QString *error)
{
    const QString path = credentialsPath();
    if (!QFile::exists(path)) {
        if (error) error->clear();
        return {};
    }
    if (!securePermissions(path, error)) return {};
    QSettings settings(path, QSettings::IniFormat);
    const QString value = settings.value(QStringLiteral("secrets/%1").arg(providerId)).toString();
    if (settings.status() != QSettings::NoError) {
        if (error) *error = QStringLiteral("Could not read the local credential file");
        return {};
    }
    if (error) error->clear();
    return value;
}

bool LibSecretStore::remove(const QString &providerId, QString *error)
{
    const QString path = credentialsPath();
    if (!QFile::exists(path)) {
        if (error) error->clear();
        return true;
    }
    QSettings settings(path, QSettings::IniFormat);
    settings.remove(QStringLiteral("secrets/%1").arg(providerId));
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        if (error) *error = QStringLiteral("Could not update the local credential file");
        return false;
    }
    return securePermissions(path, error);
}
