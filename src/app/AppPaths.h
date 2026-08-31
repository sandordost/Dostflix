#pragma once

#include <QString>

class AppPaths final
{
public:
    AppPaths();
    AppPaths(QString configRoot, QString dataRoot, QString cacheRoot);

    [[nodiscard]] QString configDir() const;
    [[nodiscard]] QString dataDir() const;
    [[nodiscard]] QString cacheDir() const;
    bool ensureExists() const;

private:
    QString m_configRoot;
    QString m_dataRoot;
    QString m_cacheRoot;
};
