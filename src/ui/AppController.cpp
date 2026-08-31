#include "ui/AppController.h"

AppController::AppController(QObject *parent)
    : QObject(parent)
{
}

bool AppController::hasActivePlayback() const
{
    return !m_activeMovieId.isEmpty();
}

QString AppController::activeTitle() const
{
    return m_activeTitle;
}

int AppController::watchedSeconds() const
{
    return m_watchedSeconds;
}

void AppController::setActivePlayback(const QString &movieId, const QString &title, int seconds)
{
    m_activeMovieId = movieId;
    m_activeTitle = title;
    m_watchedSeconds = seconds;
    emit activePlaybackChanged();
}

void AppController::clearActivePlayback()
{
    if (!hasActivePlayback()) {
        return;
    }
    m_activeMovieId.clear();
    m_activeTitle.clear();
    m_watchedSeconds = 0;
    emit activePlaybackChanged();
}
