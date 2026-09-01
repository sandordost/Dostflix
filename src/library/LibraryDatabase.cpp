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
    if (schemaVersion() < 3 && !migrateToVersionThree()) return false;
    return schemaVersion() >= 3;
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

bool LibraryDatabase::migrateToVersionThree()
{
    if (!m_database.transaction()) {
        m_lastError = m_database.lastError().text();
        return false;
    }
    QSqlQuery query(m_database);
    const bool succeeded = query.exec(QStringLiteral(
        "CREATE TABLE transfers ("
        "torrent_hash TEXT NOT NULL, "
        "file_index INTEGER NOT NULL, "
        "title TEXT NOT NULL, "
        "file_name TEXT NOT NULL, "
        "expected_size INTEGER NOT NULL, "
        "partial_path TEXT NOT NULL, "
        "final_path TEXT NOT NULL, "
        "bytes_written INTEGER NOT NULL DEFAULT 0, "
        "state TEXT NOT NULL, "
        "updated_at INTEGER NOT NULL DEFAULT (unixepoch()), "
        "PRIMARY KEY(torrent_hash, file_index))"))
        && query.exec(QStringLiteral("PRAGMA user_version = 3"));
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

namespace {
LibraryTransfer readTransfer(const QSqlQuery &query)
{
    return {query.value(0).toString(), query.value(1).toInt(),
            query.value(2).toString(), query.value(3).toString(),
            query.value(4).toLongLong(), query.value(5).toString(),
            query.value(6).toString(), query.value(7).toLongLong(),
            query.value(8).toString()};
}
}

std::optional<LibraryTransfer> LibraryDatabase::transfer(
    const QString &torrentHash, int fileIndex) const
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT torrent_hash, file_index, title, file_name, expected_size, partial_path, "
        "final_path, bytes_written, state FROM transfers "
        "WHERE torrent_hash = :hash AND file_index = :fileIndex"));
    query.bindValue(QStringLiteral(":hash"), torrentHash);
    query.bindValue(QStringLiteral(":fileIndex"), fileIndex);
    if (!query.exec() || !query.next()) return std::nullopt;
    return readTransfer(query);
}

std::optional<LibraryTransfer> LibraryDatabase::latestIncompleteTransfer() const
{
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT torrent_hash, file_index, title, file_name, expected_size, partial_path, "
            "final_path, bytes_written, state FROM transfers "
            "WHERE state != 'completed' ORDER BY updated_at DESC LIMIT 1")) || !query.next()) {
        return std::nullopt;
    }
    return readTransfer(query);
}

std::optional<LibraryTransfer> LibraryDatabase::latestTransfer() const
{
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT torrent_hash, file_index, title, file_name, expected_size, partial_path, "
            "final_path, bytes_written, state FROM transfers "
            "ORDER BY updated_at DESC LIMIT 1")) || !query.next()) {
        return std::nullopt;
    }
    return readTransfer(query);
}

bool LibraryDatabase::saveTransfer(const LibraryTransfer &transfer)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO transfers(torrent_hash, file_index, title, file_name, expected_size, "
        "partial_path, final_path, bytes_written, state, updated_at) "
        "VALUES(:hash, :fileIndex, :title, :fileName, :size, :partial, :final, :bytes, :state, unixepoch()) "
        "ON CONFLICT(torrent_hash, file_index) DO UPDATE SET "
        "title=excluded.title, file_name=excluded.file_name, expected_size=excluded.expected_size, "
        "partial_path=excluded.partial_path, final_path=excluded.final_path, "
        "bytes_written=excluded.bytes_written, state=excluded.state, updated_at=unixepoch()"));
    query.bindValue(QStringLiteral(":hash"), transfer.torrentHash);
    query.bindValue(QStringLiteral(":fileIndex"), transfer.fileIndex);
    query.bindValue(QStringLiteral(":title"), transfer.title);
    query.bindValue(QStringLiteral(":fileName"), transfer.fileName);
    query.bindValue(QStringLiteral(":size"), transfer.expectedSize);
    query.bindValue(QStringLiteral(":partial"), transfer.partialPath);
    query.bindValue(QStringLiteral(":final"), transfer.finalPath);
    query.bindValue(QStringLiteral(":bytes"), transfer.bytesWritten);
    query.bindValue(QStringLiteral(":state"), transfer.state);
    if (query.exec()) return true;
    m_lastError = query.lastError().text();
    return false;
}

bool LibraryDatabase::updateTransferProgress(const QString &torrentHash, int fileIndex,
                                             qint64 bytesWritten, const QString &state)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "UPDATE transfers SET bytes_written=:bytes, state=:state, updated_at=unixepoch() "
        "WHERE torrent_hash=:hash AND file_index=:fileIndex"));
    query.bindValue(QStringLiteral(":bytes"), bytesWritten);
    query.bindValue(QStringLiteral(":state"), state);
    query.bindValue(QStringLiteral(":hash"), torrentHash);
    query.bindValue(QStringLiteral(":fileIndex"), fileIndex);
    if (query.exec()) return true;
    m_lastError = query.lastError().text();
    return false;
}

bool LibraryDatabase::removeTransfer(const QString &torrentHash, int fileIndex)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "DELETE FROM transfers WHERE torrent_hash=:hash AND file_index=:fileIndex"));
    query.bindValue(QStringLiteral(":hash"), torrentHash);
    query.bindValue(QStringLiteral(":fileIndex"), fileIndex);
    if (query.exec()) return true;
    m_lastError = query.lastError().text();
    return false;
}

bool LibraryDatabase::removeMovieByPath(const QString &videoPath)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM movies WHERE video_path=:path"));
    query.bindValue(QStringLiteral(":path"), videoPath);
    if (query.exec()) return true;
    m_lastError = query.lastError().text();
    return false;
}
