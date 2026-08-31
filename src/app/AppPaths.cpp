#include "app/AppPaths.h"

#include <QDir>
#include <QStandardPaths>
#include <utility>

AppPaths::AppPaths()
    : AppPaths(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation),
               QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation),
               QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation))
{
}

AppPaths::AppPaths(QString configRoot, QString dataRoot, QString cacheRoot)
    : m_configRoot(std::move(configRoot))
    , m_dataRoot(std::move(dataRoot))
    , m_cacheRoot(std::move(cacheRoot))
{
}

QString AppPaths::configDir() const
{
    return QDir::cleanPath(m_configRoot + QStringLiteral("/dostflix"));
}

QString AppPaths::dataDir() const
{
    return QDir::cleanPath(m_dataRoot + QStringLiteral("/dostflix"));
}

QString AppPaths::cacheDir() const
{
    return QDir::cleanPath(m_cacheRoot + QStringLiteral("/dostflix"));
}

bool AppPaths::ensureExists() const
{
    return QDir().mkpath(configDir()) && QDir().mkpath(dataDir()) && QDir().mkpath(cacheDir());
}
