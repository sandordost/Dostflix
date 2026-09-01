#pragma once

#include "library/LocalLibraryModel.h"

#include <QObject>
#include <QUrl>

class AppSettings;
class LibraryDatabase;

class LibraryManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(LocalLibraryModel *model READ model CONSTANT)
    Q_PROPERTY(QString directory READ directory NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(int count READ count NOTIFY stateChanged)

public:
    LibraryManager(AppSettings &settings, LibraryDatabase &database,
                   QString defaultDirectory, QObject *parent = nullptr);
    LocalLibraryModel *model();
    QString directory() const;
    QString errorMessage() const;
    int count() const;

    Q_INVOKABLE bool setDirectory(const QUrl &directoryUrl);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void play(int row, bool restart = false);
    void recordPlaybackProgress(int watchedSeconds, int durationSeconds, bool force = false);
    Q_INVOKABLE void clearPlaybackSession();

signals:
    void stateChanged();
    void playbackRequested(const QUrl &fileUrl, const QString &title, int startSeconds);

private:
    void reload();
    static bool isVideoFile(const QString &path);
    static QString displayTitle(const QString &path);

    AppSettings &m_settings;
    LibraryDatabase &m_database;
    QString m_directory;
    QString m_error;
    QString m_activeVideoPath;
    int m_lastPersistedSeconds = 0;
    LocalLibraryModel m_model;
};
