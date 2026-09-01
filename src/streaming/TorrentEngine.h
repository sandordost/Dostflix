#pragma once

#include "streaming/TorrentFileModel.h"

#include <QObject>
#include <QTimer>
#include <memory>

class TorrentEngine final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TorrentFileModel *videoFiles READ videoFiles CONSTANT)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool needsFileSelection READ needsFileSelection NOTIFY stateChanged)
    Q_PROPERTY(QString title READ title NOTIFY stateChanged)
    Q_PROPERTY(QString selectedFileName READ selectedFileName NOTIFY stateChanged)
    Q_PROPERTY(QString selectedFilePath READ selectedFilePath NOTIFY stateChanged)
    Q_PROPERTY(QString stateLabel READ stateLabel NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(double progress READ progress NOTIFY statisticsChanged)
    Q_PROPERTY(qint64 downloadRate READ downloadRate NOTIFY statisticsChanged)
    Q_PROPERTY(int peerCount READ peerCount NOTIFY statisticsChanged)
    Q_PROPERTY(double bufferSeconds READ bufferSeconds NOTIFY statisticsChanged)
    Q_PROPERTY(double estimatedWaitSeconds READ estimatedWaitSeconds NOTIFY statisticsChanged)
    Q_PROPERTY(bool bufferReady READ bufferReady NOTIFY statisticsChanged)

public:
    explicit TorrentEngine(QString downloadDir, QObject *parent = nullptr);
    ~TorrentEngine() override;

    [[nodiscard]] TorrentFileModel *videoFiles();
    [[nodiscard]] bool active() const;
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
    struct Impl;
    void poll();
    void collectVideoFiles();
    void applyFileSelection(const TorrentVideoFile &file);
    void finalizeFileSelection();
    void updateStatistics();
    void fail(QString error);

    QString m_downloadDir;
    QString m_title;
    QString m_selectedFileName;
    QString m_stateLabel;
    QString m_error;
    TorrentFileModel m_videoFiles;
    QTimer m_pollTimer;
    std::unique_ptr<Impl> m_impl;
    bool m_networkReady = false;
    bool m_active = false;
    bool m_needsFileSelection = false;
    int m_selectedTorrentIndex = -1;
    qint64 m_selectedFileSize = 0;
    double m_progress = 0.0;
    qint64 m_downloadRate = 0;
    int m_peerCount = 0;
    double m_bufferSeconds = 0.0;
    double m_estimatedWaitSeconds = 0.0;
    bool m_bufferReady = false;
    bool m_waitingFilePriorities = false;
};
