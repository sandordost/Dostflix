#pragma once

#include "library/LibraryDatabase.h"

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QQueue>
#include <QUrl>

class LibraryManager;
class ProviderManager;
class QNetworkReply;

class LibraryMetadataManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString stateLabel READ stateLabel NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
public:
    LibraryMetadataManager(LibraryDatabase &database, LibraryManager &library,
                           ProviderManager &providers, QString dataDir,
                           QUrl apiBase = QUrl(QStringLiteral("https://api.themoviedb.org/3/")),
                           QUrl imageBase = QUrl(QStringLiteral("https://image.tmdb.org/t/p/w500/")),
                           QObject *parent = nullptr);
    bool busy() const;
    QString stateLabel() const;
    QString errorMessage() const;
    void setNetworkReady(bool ready);
    Q_INVOKABLE void refresh();
    void cancel();
signals:
    void stateChanged();
private:
    void searchNext();
    void fetchDetails(int tmdbId);
    void fetchPoster(const QJsonObject &details);
    void finishCurrent(const QJsonObject &details, const QString &posterPath);
    QNetworkRequest request(const QUrl &url) const;
    void failCurrent(QString error);
    LibraryDatabase &m_database;
    LibraryManager &m_library;
    ProviderManager &m_providers;
    QString m_dataDir;
    QUrl m_apiBase;
    QUrl m_imageBase;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QQueue<LibraryMovie> m_queue;
    LibraryMovie m_current;
    QString m_state;
    QString m_error;
    bool m_networkReady = false;
};
