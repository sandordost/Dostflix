#pragma once

#include <QQuickFramebufferObject>
#include <QString>
#include <QtQmlIntegration>
#include <memory>

struct mpv_handle;
struct MpvSharedState;

class MpvPlayer : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool hasActivePlayback READ hasActivePlayback NOTIFY activePlaybackChanged)
    Q_PROPERTY(QString activeTitle READ activeTitle NOTIFY activePlaybackChanged)
    Q_PROPERTY(int watchedSeconds READ watchedSeconds NOTIFY positionChanged)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(bool buffering READ buffering NOTIFY bufferingChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(bool renderReady READ renderReady NOTIFY renderReadyChanged)

public:
    explicit MpvPlayer(QQuickItem *parent = nullptr);
    ~MpvPlayer() override;

    Renderer *createRenderer() const override;
    bool hasActivePlayback() const;
    QString activeTitle() const;
    int watchedSeconds() const;
    double position() const;
    double duration() const;
    bool paused() const;
    bool buffering() const;
    double volume() const;
    QString errorMessage() const;
    bool renderReady() const;

    Q_INVOKABLE void play(const QString &url, const QString &title);
    Q_INVOKABLE void togglePaused();
    Q_INVOKABLE void seek(double offsetSeconds);
    Q_INVOKABLE void setPosition(double seconds);
    Q_INVOKABLE void setVolume(double value);
    Q_INVOKABLE void stop();
    void processEvents();

signals:
    void activePlaybackChanged();
    void positionChanged();
    void durationChanged();
    void pausedChanged();
    void bufferingChanged();
    void volumeChanged();
    void errorChanged();
    void renderReadyChanged();

private:
    friend class MpvRenderer;
    mpv_handle *handle() const;
    void setRenderReady();
    void submitPendingLoad();
    void setError(const QString &message);

    mpv_handle *m_handle = nullptr;
    std::shared_ptr<MpvSharedState> m_state;
    QString m_pendingUrl;
    QString m_activeTitle;
    QString m_error;
    double m_position = 0.0;
    double m_duration = 0.0;
    double m_volume = 100.0;
    bool m_renderReady = false;
    bool m_active = false;
    bool m_paused = false;
    bool m_buffering = false;
};
