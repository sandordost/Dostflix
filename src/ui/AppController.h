#pragma once

#include <QObject>

class AppController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasActivePlayback READ hasActivePlayback NOTIFY activePlaybackChanged)
    Q_PROPERTY(QString activeTitle READ activeTitle NOTIFY activePlaybackChanged)
    Q_PROPERTY(int watchedSeconds READ watchedSeconds NOTIFY activePlaybackChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    [[nodiscard]] bool hasActivePlayback() const;
    [[nodiscard]] QString activeTitle() const;
    [[nodiscard]] int watchedSeconds() const;
    Q_INVOKABLE void setActivePlayback(const QString &movieId, const QString &title, int seconds);
    Q_INVOKABLE void clearActivePlayback();

signals:
    void activePlaybackChanged();

private:
    QString m_activeMovieId;
    QString m_activeTitle;
    int m_watchedSeconds = 0;
};
