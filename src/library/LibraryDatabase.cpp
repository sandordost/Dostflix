#include "library/LibraryDatabase.h"

#include <QSqlError>
#include <QSqlQuery>
#include <utility>

LibraryDatabase::LibraryDatabase(QString fileName, QString connectionName)
    : m_fileName(std::move(fileName))
    , m_connectionName(std::move(connectionName))
{
}

LibraryDatabase::~LibraryDatabase()
{
    if (m_database.isValid()) {
        m_database.close();
    }
    m_database = {};
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool LibraryDatabase::open()
{
    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setDatabaseName(m_fileName);
    if (!m_database.open()) {
        m_lastError = m_database.lastError().text();
        return false;
    }

    QSqlQuery pragma(m_database);
    if (!pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        m_lastError = pragma.lastError().text();
        return false;
    }
    if (schemaVersion() < 1 && !migrateToVersionOne()) return false;
    if (schemaVersion() < 2 && !migrateToVersionTwo()) return false;
    return schemaVersion() >= 2;
}

int LibraryDatabase::schemaVersion() const
{
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("PRAGMA user_version")) || !query.next()) {
        return 0;
    }
    return query.value(0).toInt();
}

bool LibraryDatabase::migrateToVersionOne()
{
    if (!m_database.transaction()) {
        m_lastError = m_database.lastError().text();
        return false;
    }

    QSqlQuery query(m_database);
    const bool statementsSucceeded = query.exec(QStringLiteral(
        "CREATE TABLE movies ("
        "id INTEGER PRIMARY KEY, "
        "title TEXT NOT NULL, "
        "year INTEGER, "
        "poster_path TEXT, "
        "video_path TEXT, "
        "watched_seconds INTEGER NOT NULL DEFAULT 0, "
        "duration_seconds INTEGER NOT NULL DEFAULT 0)"))
        && query.exec(QStringLiteral("PRAGMA user_version = 1"));

    if (!statementsSucceeded || !m_database.commit()) {
        m_lastError = statementsSucceeded ? m_database.lastError().text() : query.lastError().text();
        m_database.rollback();
        return false;
    }
    return true;
}

bool LibraryDatabase::migrateToVersionTwo()
{
    if (!m_database.transaction()) {
        m_lastError = m_database.lastError().text();
        return false;
    }
    QSqlQuery query(m_database);
    const bool succeeded = query.exec(QStringLiteral(
        "CREATE UNIQUE INDEX movies_video_path ON movies(video_path)"))
        && query.exec(QStringLiteral("PRAGMA user_version = 2"));
    if (!succeeded || !m_database.commit()) {
        m_lastError = succeeded ? m_database.lastError().text() : query.lastError().text();
        m_database.rollback();
        return false;
    }
    return true;
}

QString LibraryDatabase::lastError() const
{
    return m_lastError;
}

QSqlDatabase LibraryDatabase::connection() const
{
    return m_database;
}

QList<LibraryMovie> LibraryDatabase::movies() const
{
    QList<LibraryMovie> result;
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT id, title, year, poster_path, video_path, watched_seconds, duration_seconds "
            "FROM movies ORDER BY title COLLATE NOCASE"))) return result;
    while (query.next()) {
        result.push_back({query.value(0).toLongLong(), query.value(1).toString(),
                          query.value(2).toInt(), query.value(3).toString(),
                          query.value(4).toString(), query.value(5).toInt(),
                          query.value(6).toInt()});
    }
    return result;
}

bool LibraryDatabase::upsertMovie(const QString &title, const QString &videoPath)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO movies(title, video_path) VALUES(:title, :path) "
        "ON CONFLICT(video_path) DO UPDATE SET title = excluded.title"));
    query.bindValue(QStringLiteral(":title"), title);
    query.bindValue(QStringLiteral(":path"), videoPath);
    if (query.exec()) return true;
    m_lastError = query.lastError().text();
    return false;
}
