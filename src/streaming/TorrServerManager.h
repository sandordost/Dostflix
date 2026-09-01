#pragma once

#include "streaming/TorrentFileModel.h"

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <QUrlQuery>
#include <functional>

class QNetworkReply;

class TorrServerManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TorrentFileModel *videoFiles READ videoFiles CONSTANT)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool backendReady READ backendReady NOTIFY stateChanged)
    Q_PROPERTY(bool needsFileSelection READ needsFileSelection NOTIFY stateChanged)
    Q_PROPERTY(QString title READ title NOTIFY stateChanged)
    Q_PROPERTY(QString selectedFileName READ selectedFileName NOTIFY stateChanged)
    Q_PROPERTY(QString streamUrl READ streamUrl NOTIFY stateChanged)
    Q_PROPERTY(QString stateLabel READ stateLabel NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(double progress READ progress NOTIFY statisticsChanged)
    Q_PROPERTY(qint64 downloadRate READ downloadRate NOTIFY statisticsChanged)
    Q_PROPERTY(int peerCount READ peerCount NOTIFY statisticsChanged)
    Q_PROPERTY(int seedCount READ seedCount NOTIFY statisticsChanged)
    Q_PROPERTY(double distributedCopies READ distributedCopies NOTIFY statisticsChanged)
    Q_PROPERTY(double bufferSeconds READ bufferSeconds NOTIFY statisticsChanged)
    Q_PROPERTY(double estimatedWaitSeconds READ estimatedWaitSeconds NOTIFY statisticsChanged)
    Q_PROPERTY(bool bufferReady READ bufferReady NOTIFY statisticsChanged)

public:
    explicit TorrServerManager(QString dataDir, QObject *parent = nullptr);
    ~TorrServerManager() override;

    TorrentFileModel *videoFiles();
    bool active() const;
    bool backendReady() const;
    bool needsFileSelection() const;
    QString title() const;
    QString selectedFileName() const;
    QString streamUrl() const;
    QString stateLabel() const;
    QString errorMessage() const;
    double progress() const;
    qint64 downloadRate() const;
    int peerCount() const;
    int seedCount() const;
    double distributedCopies() const;
    double bufferSeconds() const;
    double estimatedWaitSeconds() const;
    bool bufferReady() const;

    void setNetworkReady(bool ready);
    Q_INVOKABLE void startMagnet(const QString &title, const QString &magnetUrl);
    void startTorrentData(const QString &title, const QByteArray &torrentData);
    void resumeStoredTorrent(const QString &title, const QString &torrentHash,
                             int fileIndex, const QString &fileName, qint64 expectedSize);
    Q_INVOKABLE void selectVideoFile(int row);
    Q_INVOKABLE void cancel();
    void shutdown();

signals:
    void stateChanged();
    void statisticsChanged();
    void retentionSourceReady(const QString &title, const QString &torrentHash,
                              int fileIndex, const QString &fileName,
                              qint64 expectedSize, const QUrl &sourceUrl);

private:
    void startDaemon();
    void stopDaemon(bool force);
    void poll();
    void probeApi();
    void beginRelease(QString title, QString magnetUrl, QByteArray torrentData);
    void retirePreviousOrSubmit();
    void submitPendingRelease();
    void requestStatus();
    void parseStatus(const QByteArray &body);
    void startPreload();
    void clearTransferState();
    void fail(QString error);
    QNetworkRequest request(const QString &path) const;
    void get(const QString &path, const QUrlQuery &query,
             std::function<void(QNetworkReply *)> finished);
    void postJson(const QString &path, const QJsonObject &body,
                  std::function<void(QNetworkReply *)> finished);

    QString m_dataDir;
    QString m_logPath;
    QProcess m_process;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QPointer<QNetworkReply> m_preloadReply;
    QTimer m_pollTimer;
    QTimer m_startupTimer;
    QUrl m_baseUrl;
    TorrentFileModel m_videoFiles;
    QString m_title;
    QString m_hash;
    QString m_pendingMagnet;
    QByteArray m_pendingTorrent;
    QString m_retiringHash;
    QString m_selectedFileName;
    QString m_stateLabel;
    QString m_error;
    bool m_networkReady = false;
    bool m_daemonReady = false;
    bool m_stopping = false;
    bool m_active = false;
    bool m_needsFileSelection = false;
    bool m_preloadStarted = false;
    bool m_bufferReady = false;
    bool m_dropInProgress = false;
    bool m_resumePending = false;
    bool m_retentionAnnounced = false;
    int m_selectedFileId = -1;
    qint64 m_selectedFileSize = 0;
    double m_progress = 0.0;
    qint64 m_downloadRate = 0;
    int m_peerCount = 0;
    int m_seedCount = 0;
    double m_bufferSeconds = 0.0;
    double m_estimatedWaitSeconds = 0.0;
};
