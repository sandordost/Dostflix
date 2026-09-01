#pragma once

#include "library/LibraryDatabase.h"

#include <QElapsedTimer>
#include <QFile>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QUrl>

class LibraryManager;
class QNetworkReply;

class DownloadManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool hasPending READ hasPending NOTIFY stateChanged)
    Q_PROPERTY(bool hasTransfer READ hasTransfer NOTIFY stateChanged)
    Q_PROPERTY(bool playable READ playable NOTIFY stateChanged)
    Q_PROPERTY(QString title READ title NOTIFY stateChanged)
    Q_PROPERTY(qint64 bytesWritten READ bytesWritten NOTIFY stateChanged)
    Q_PROPERTY(qint64 expectedSize READ expectedSize NOTIFY stateChanged)
    Q_PROPERTY(double progress READ progress NOTIFY stateChanged)
    Q_PROPERTY(QString stateLabel READ stateLabel NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)

public:
    DownloadManager(LibraryDatabase &database, LibraryManager &library,
                    QObject *parent = nullptr);
    ~DownloadManager() override;

    bool active() const;
    bool hasPending() const;
    bool hasTransfer() const;
    bool playable() const;
    QString title() const;
    qint64 bytesWritten() const;
    qint64 expectedSize() const;
    double progress() const;
    QString stateLabel() const;
    QString errorMessage() const;

    void setNetworkReady(bool ready);
    void beginTransfer(const QString &title, const QString &torrentHash, int fileIndex,
                       const QString &fileName, qint64 expectedSize, const QUrl &sourceUrl);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void play();
    Q_INVOKABLE void remove();
    bool playMatchingRelease(const QString &title, const QString &magnetUrl);

signals:
    void stateChanged();
    void resumeRequested(const QString &title, const QString &torrentHash, int fileIndex,
                         const QString &fileName, qint64 expectedSize);
    void torrentPlaybackRequested(const QString &title, const QString &torrentHash,
                                  int fileIndex, const QString &fileName,
                                  qint64 expectedSize);
    void localPlaybackRequested(const QUrl &fileUrl, const QString &title);
    void torrentRemovalRequested(const QString &torrentHash);

private:
    void loadPending();
    void startRequest();
    bool validateResponse();
    void writeAvailable();
    void finishRequest();
    void completeTransfer();
    bool persist(const QString &state);
    void fail(QString error);
    QString chooseFinalPath(const QString &fileName, const QString &torrentHash) const;
    bool pathsAreSafe() const;

    LibraryDatabase &m_database;
    LibraryManager &m_library;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QFile m_file;
    LibraryTransfer m_transfer;
    QUrl m_sourceUrl;
    QString m_stateLabel;
    QString m_error;
    QElapsedTimer m_persistTimer;
    qint64 m_requestOffset = 0;
    bool m_networkReady = false;
    bool m_active = false;
    bool m_headersValidated = false;
    bool m_pausing = false;
};
