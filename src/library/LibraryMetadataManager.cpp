#include "library/LibraryMetadataManager.h"

#include "library/LibraryManager.h"
#include "providers/ProviderManager.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QUrlQuery>
#include <utility>

LibraryMetadataManager::LibraryMetadataManager(
    LibraryDatabase &database, LibraryManager &library, ProviderManager &providers,
    QString dataDir, QUrl apiBase, QUrl imageBase, QObject *parent)
    : QObject(parent), m_database(database), m_library(library), m_providers(providers),
      m_dataDir(std::move(dataDir)), m_apiBase(std::move(apiBase)),
      m_imageBase(std::move(imageBase)) {}

bool LibraryMetadataManager::busy() const { return !m_reply.isNull() || !m_queue.isEmpty(); }
QString LibraryMetadataManager::stateLabel() const { return m_state; }
QString LibraryMetadataManager::errorMessage() const { return m_error; }

void LibraryMetadataManager::setNetworkReady(bool ready)
{
    if (m_networkReady == ready) return;
    m_networkReady = ready;
    ready ? refresh() : cancel();
}

void LibraryMetadataManager::refresh()
{
    if (!m_networkReady || !m_providers.hasTmdbToken() || m_reply) return;
    m_queue.clear();
    for (const LibraryMovie &movie : m_database.movies())
        if (movie.tmdbId <= 0) m_queue.enqueue(movie);
    m_error.clear();
    searchNext();
}

void LibraryMetadataManager::cancel()
{
    if (m_reply) {
        QNetworkReply *reply = m_reply; m_reply = nullptr;
        reply->abort(); reply->deleteLater();
    }
    m_queue.clear(); m_state.clear();
    emit stateChanged();
}

void LibraryMetadataManager::searchNext()
{
    if (!m_networkReady || m_reply) return;
    if (m_queue.isEmpty()) { m_state.clear(); emit stateChanged(); return; }
    m_current = m_queue.dequeue();
    m_state = tr("Matching %1 with TMDB…").arg(m_current.title);
    QUrl url = m_apiBase.resolved(QUrl(QStringLiteral("search/movie")));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("query"), m_current.title);
    if (m_current.year > 0) query.addQueryItem(QStringLiteral("year"), QString::number(m_current.year));
    query.addQueryItem(QStringLiteral("language"), QStringLiteral("nl-NL"));
    query.addQueryItem(QStringLiteral("include_adult"), QStringLiteral("false"));
    url.setQuery(query);
    QNetworkReply *reply = m_network.get(request(url)); m_reply = reply;
    emit stateChanged();
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        const QJsonArray results = QJsonDocument::fromJson(reply->readAll()).object()
                                       .value(QStringLiteral("results")).toArray();
        if (reply->error() != QNetworkReply::NoError || results.isEmpty()) {
            failCurrent(reply->error() == QNetworkReply::NoError
                ? tr("No TMDB match for %1").arg(m_current.title) : reply->errorString());
        } else {
            int selectedId = 0;
            for (const QJsonValue &value : results) {
                const QJsonObject candidate = value.toObject();
                const int candidateYear = candidate.value(QStringLiteral("release_date"))
                                              .toString().left(4).toInt();
                if (selectedId == 0 || (m_current.year > 0 && candidateYear == m_current.year))
                    selectedId = candidate.value(QStringLiteral("id")).toInt();
                if (m_current.year > 0 && candidateYear == m_current.year) break;
            }
            selectedId > 0 ? fetchDetails(selectedId)
                           : failCurrent(tr("TMDB returned no usable movie match"));
        }
        reply->deleteLater();
    });
}

void LibraryMetadataManager::fetchDetails(int tmdbId)
{
    m_state = tr("Loading movie details…");
    QUrl url = m_apiBase.resolved(QUrl(QStringLiteral("movie/%1").arg(tmdbId)));
    QUrlQuery query; query.addQueryItem(QStringLiteral("language"), QStringLiteral("nl-NL"));
    url.setQuery(query);
    QNetworkReply *reply = m_network.get(request(url)); m_reply = reply;
    emit stateChanged();
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        const QJsonObject details = QJsonDocument::fromJson(reply->readAll()).object();
        if (reply->error() != QNetworkReply::NoError || details.value(QStringLiteral("id")).toInt() <= 0)
            failCurrent(reply->errorString());
        else fetchPoster(details);
        reply->deleteLater();
    });
}

void LibraryMetadataManager::fetchPoster(const QJsonObject &details)
{
    const QString remotePath = details.value(QStringLiteral("poster_path")).toString();
    if (remotePath.isEmpty()) { finishCurrent(details, {}); return; }
    const QString directory = QDir(m_dataDir).filePath(QStringLiteral("metadata/posters"));
    if (!QDir().mkpath(directory)) { failCurrent(tr("Could not create poster cache")); return; }
    const QString localPath = QDir(directory).filePath(
        QString::number(details.value(QStringLiteral("id")).toInt()) + QStringLiteral(".jpg"));
    QUrl url = m_imageBase.resolved(QUrl(remotePath.startsWith('/') ? remotePath.mid(1) : remotePath));
    QNetworkRequest posterRequest(url);
    posterRequest.setTransferTimeout(15'000);
    QNetworkReply *reply = m_network.get(posterRequest); m_reply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, details, localPath] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        QSaveFile poster(localPath); const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError || body.isEmpty()
            || !poster.open(QIODevice::WriteOnly) || poster.write(body) != body.size()
            || !poster.commit()) finishCurrent(details, {});
        else finishCurrent(details, localPath);
        reply->deleteLater();
    });
}

void LibraryMetadataManager::finishCurrent(const QJsonObject &details, const QString &posterPath)
{
    const QString title = details.value(QStringLiteral("title")).toString(m_current.title);
    const int year = details.value(QStringLiteral("release_date")).toString().left(4).toInt();
    if (!m_database.updateMovieMetadata(
            m_current.videoPath, details.value(QStringLiteral("id")).toInt(),
            details.value(QStringLiteral("imdb_id")).toString(), title,
            year > 0 ? year : m_current.year, posterPath,
            details.value(QStringLiteral("runtime")).toInt() * 60,
            details.value(QStringLiteral("overview")).toString())) m_error = m_database.lastError();
    m_library.refresh();
    searchNext();
}

QNetworkRequest LibraryMetadataManager::request(const QUrl &url) const
{
    QNetworkRequest result(url);
    result.setRawHeader("Accept", "application/json");
    result.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_providers.tmdbToken().toUtf8());
    result.setTransferTimeout(15'000);
    return result;
}

void LibraryMetadataManager::failCurrent(QString error)
{
    m_error = std::move(error);
    searchNext();
}
