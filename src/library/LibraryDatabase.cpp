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
    return schemaVersion() >= 1 || migrateToVersionOne();
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

QString LibraryDatabase::lastError() const
{
    return m_lastError;
}

QSqlDatabase LibraryDatabase::connection() const
{
    return m_database;
}
