#pragma once

#include <QSqlDatabase>
#include <QString>

class LibraryDatabase final
{
public:
    LibraryDatabase(QString fileName, QString connectionName);
    ~LibraryDatabase();

    bool open();
    [[nodiscard]] int schemaVersion() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] QSqlDatabase connection() const;

private:
    bool migrateToVersionOne();

    QString m_fileName;
    QString m_connectionName;
    QSqlDatabase m_database;
    QString m_lastError;
};
