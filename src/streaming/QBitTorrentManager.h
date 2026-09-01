#pragma once

#include "streaming/TorrentFileModel.h"

#include <QHash>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <QUrlQuery>
#include <functional>

class QNetworkReply;

class QBitTorrentManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TorrentFileModel *videoFiles READ videoFiles CONSTANT)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool backendReady READ backendReady NOTIFY stateChanged)
    Q_PROPERTY(bool needsFileSelection READ needsFileSelection NOTIFY stateChanged)
    Q_PROPERTY(QString title READ title NOTIFY stateChanged)
    Q_PROPERTY(QString selectedFileName READ selectedFileName NOTIFY stateChanged)
    Q_PROPERTY(QString selectedFilePath READ selectedFilePath NOTIFY stateChanged)
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
    explicit QBitTorrentManager(QString dataDir, QString downloadDir,
                                QObject *parent = nullptr);
    ~QBitTorrentManager() override;

    [[nodiscard]] TorrentFileModel *videoFiles();
    [[nodiscard]] bool active() const;
    [[nodiscard]] bool backendReady() const;
    [[nodiscard]] bool needsFileSelection() const;
    [[nodiscard]] QString title() const;
    [[nodiscard]] QString selectedFileName() const;
    [[nodiscard]] QString selectedFilePath() const;
    [[nodiscard]] qint64 selectedFileSize() const;
    [[nodiscard]] QString stateLabel() const;
    [[nodiscard]] QString errorMessage() const;
    [[nodiscard]] double progress() const;
    [[nodiscard]] qint64 downloadRate() const;
    [[nodiscard]] int peerCount() const;
    [[nodiscard]] int seedCount() const;
    [[nodiscard]] double distributedCopies() const;
    [[nodiscard]] double bufferSeconds() const;
    [[nodiscard]] double estimatedWaitSeconds() const;
    [[nodiscard]] bool bufferReady() const;

    void setNetworkReady(bool ready);
    [[nodiscard]] bool isRangeAvailable(qint64 offset, qint64 length) const;
    void prioritizeRange(qint64 offset, qint64 length);
    Q_INVOKABLE void startMagnet(const QString &title, const QString &magnetUrl);
    void startTorrentData(const QString &title, const QByteArray &torrentData);
    Q_INVOKABLE void selectVideoFile(int row);
    Q_INVOKABLE void cancel();
    void shutdown();

signals:
    void stateChanged();
    void statisticsChanged();

private:
    struct ApiFile final { int index; QString path; qint64 size; qint64 offset; };
    void startDaemon();
    void stopDaemon(bool force);
    bool writeConfiguration();
    void poll();
    void probeApi();
    void submitPendingRelease();
    void requestTorrentInfo();
    void requestProperties();
    void requestFiles();
    void requestPieceStates();
    void applyFileSelection(const TorrentVideoFile &file);
    void startSelectedFile();
    void updateBuffer();
    void clearTransferState();
    void fail(QString error);
    [[nodiscard]] QNetworkRequest apiRequest(const QString &path) const;
    void get(const QString &path, const QUrlQuery &query,
             std::function<void(QNetworkReply *)> finished);
    void post(const QString &path, const QUrlQuery &form,
              std::function<void(QNetworkReply *)> finished);

    QString m_dataDir;
    QString m_downloadDir;
    QString m_profileDir;
    QProcess m_process;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QTimer m_pollTimer;
    QUrl m_apiBase;
    TorrentFileModel m_videoFiles;
    QList<ApiFile> m_apiFiles;
    QHash<int, qint64> m_fileOffsets;
    QList<int> m_pieceStates;
    QString m_title;
    QString m_hash;
    QString m_tag;
    QString m_pendingMagnet;
    QByteArray m_pendingTorrent;
    QString m_selectedFileName;
    QString m_stateLabel;
    QString m_error;
    bool m_networkReady = false;
    bool m_daemonReady = false;
    bool m_stopping = false;
    bool m_active = false;
    bool m_needsFileSelection = false;
    bool m_bufferReady = false;
    int m_selectedTorrentIndex = -1;
    qint64 m_selectedFileSize = 0;
    qint64 m_selectedFileOffset = 0;
    qint64 m_pieceSize = 0;
    double m_progress = 0.0;
    qint64 m_downloadRate = 0;
    int m_peerCount = 0;
    int m_seedCount = 0;
    double m_distributedCopies = 0.0;
    double m_bufferSeconds = 0.0;
    double m_estimatedWaitSeconds = 0.0;
};
