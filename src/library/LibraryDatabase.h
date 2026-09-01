#pragma once

#include <QSqlDatabase>
#include <QList>
#include <QString>
#include <optional>

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

struct LibraryTransfer final
{
    QString torrentHash;
    int fileIndex = -1;
    QString title;
    QString fileName;
    qint64 expectedSize = 0;
    QString partialPath;
    QString finalPath;
    qint64 bytesWritten = 0;
    QString state;
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
    [[nodiscard]] std::optional<LibraryTransfer> transfer(
        const QString &torrentHash, int fileIndex) const;
    [[nodiscard]] std::optional<LibraryTransfer> latestIncompleteTransfer() const;
    [[nodiscard]] std::optional<LibraryTransfer> latestTransfer() const;
    bool saveTransfer(const LibraryTransfer &transfer);
    bool updateTransferProgress(const QString &torrentHash, int fileIndex,
                                qint64 bytesWritten, const QString &state);
    bool removeTransfer(const QString &torrentHash, int fileIndex);
    bool removeMovieByPath(const QString &videoPath);

private:
    bool migrateToVersionOne();
    bool migrateToVersionTwo();
    bool migrateToVersionThree();

    QString m_fileName;
    QString m_connectionName;
    QSqlDatabase m_database;
    QString m_lastError;
};
