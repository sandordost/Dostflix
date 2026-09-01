#pragma once

#include <QSqlDatabase>
#include <QList>
#include <QString>

struct LibraryMovie final
{
    qint64 id = 0;
    QString title;
    int year = 0;
    QString posterPath;
    QString videoPath;
    int watchedSeconds = 0;
    int durationSeconds = 0;
};

class LibraryDatabase final
{
public:
    LibraryDatabase(QString fileName, QString connectionName);
    ~LibraryDatabase();

    bool open();
    [[nodiscard]] int schemaVersion() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] QSqlDatabase connection() const;
    [[nodiscard]] QList<LibraryMovie> movies() const;
    bool upsertMovie(const QString &title, const QString &videoPath);

private:
    bool migrateToVersionOne();
    bool migrateToVersionTwo();

    QString m_fileName;
    QString m_connectionName;
    QSqlDatabase m_database;
    QString m_lastError;
};
