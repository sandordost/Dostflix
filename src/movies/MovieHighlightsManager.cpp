#include "movies/MovieHighlightsManager.h"

#include "providers/ProviderManager.h"

#include <QDate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QUrlQuery>
#include <algorithm>
#include <utility>

namespace {
std::vector<Movie> parseMovies(const QByteArray &payload, const bool shuffle)
{
    const QJsonArray results = QJsonDocument::fromJson(payload).object()
                                   .value(QStringLiteral("results")).toArray();
    std::vector<Movie> movies;
    movies.reserve(static_cast<std::size_t>(std::min<qsizetype>(14, results.size())));
    for (const QJsonValue &value : results) {
        const QJsonObject item = value.toObject();
        const QString title = item.value(QStringLiteral("title")).toString().trimmed();
        if (title.isEmpty()) continue;
        const QString releaseDate = item.value(QStringLiteral("release_date")).toString();
        const QString posterPath = item.value(QStringLiteral("poster_path")).toString();
        Movie movie;
        movie.id = QString::number(item.value(QStringLiteral("id")).toInteger());
        movie.title = title;
        movie.year = releaseDate.left(4).toInt();
        if (!posterPath.isEmpty())
            movie.posterUrl = QStringLiteral("https://image.tmdb.org/t/p/w500") + posterPath;
        movie.sourceLabel = QStringLiteral("TMDB");
        movie.rating = item.value(QStringLiteral("vote_average")).toDouble();
        movies.push_back(std::move(movie));
    }
    if (shuffle)
        std::shuffle(movies.begin(), movies.end(), *QRandomGenerator::global());
    if (movies.size() > 12) movies.resize(12);
    return movies;
}
}

MovieHighlightsManager::MovieHighlightsManager(ProviderManager &providers, QUrl apiBase,
                                               QObject *parent)
    : QObject(parent), m_providers(providers), m_apiBase(std::move(apiBase)),
      m_year(QDate::currentDate().year())
{
    connect(&m_providers, &ProviderManager::tmdbTokenChanged, this, [this] {
        if (!configured()) {
            cancelReplies();
            m_trending.clear();
            m_bestOfYear.clear();
            m_highRatings.clear();
            m_error.clear();
            emit stateChanged();
            return;
        }
        refresh();
    });
}

MovieListModel *MovieHighlightsManager::trendingModel() { return &m_trending; }
MovieListModel *MovieHighlightsManager::bestOfYearModel() { return &m_bestOfYear; }
MovieListModel *MovieHighlightsManager::highRatingsModel() { return &m_highRatings; }
bool MovieHighlightsManager::busy() const { return m_pending > 0; }
bool MovieHighlightsManager::configured() const { return m_providers.hasTmdbToken(); }
QString MovieHighlightsManager::errorMessage() const { return m_error; }
int MovieHighlightsManager::bestOfYear() const { return m_year; }

void MovieHighlightsManager::setNetworkReady(const bool ready)
{
    if (m_networkReady == ready) return;
    m_networkReady = ready;
    if (!ready) {
        cancelReplies();
        m_trending.clear();
        m_bestOfYear.clear();
        m_highRatings.clear();
        emit stateChanged();
        return;
    }
    refresh();
}

void MovieHighlightsManager::refresh()
{
    if (!m_networkReady || !configured()) return;
    cancelReplies();
    m_error.clear();
    ++m_generation;
    QUrlQuery trending;
    trending.addQueryItem(QStringLiteral("language"), QStringLiteral("nl-NL"));
    request(Shelf::Trending, QStringLiteral("trending/movie/week"), trending);

    QUrlQuery best;
    best.addQueryItem(QStringLiteral("language"), QStringLiteral("nl-NL"));
    best.addQueryItem(QStringLiteral("primary_release_year"), QString::number(m_year));
    best.addQueryItem(QStringLiteral("sort_by"), QStringLiteral("vote_average.desc"));
    best.addQueryItem(QStringLiteral("vote_count.gte"), QStringLiteral("100"));
    request(Shelf::BestOfYear, QStringLiteral("discover/movie"), best);

    QUrlQuery highlyRated;
    highlyRated.addQueryItem(QStringLiteral("language"), QStringLiteral("nl-NL"));
    highlyRated.addQueryItem(QStringLiteral("vote_average.gte"), QStringLiteral("8"));
    highlyRated.addQueryItem(QStringLiteral("vote_count.gte"), QStringLiteral("1000"));
    highlyRated.addQueryItem(QStringLiteral("sort_by"), QStringLiteral("popularity.desc"));
    request(Shelf::HighRatings, QStringLiteral("discover/movie"), highlyRated);
    emit stateChanged();
}

void MovieHighlightsManager::request(const Shelf shelf, const QString &path,
                                     const QUrlQuery &query)
{
    QUrl url = m_apiBase.resolved(QUrl(path));
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + m_providers.tmdbToken().toUtf8());
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(15'000);
    QNetworkReply *reply = m_network.get(request);
    reply->setProperty("generation", m_generation);
    m_replies.append(reply);
    ++m_pending;
    connect(reply, &QNetworkReply::finished, this,
            [this, shelf, reply] { complete(shelf, reply); });
}

void MovieHighlightsManager::complete(const Shelf shelf, QNetworkReply *reply)
{
    m_replies.removeAll(reply);
    if (reply->property("generation").toInt() != m_generation) {
        reply->deleteLater();
        return;
    }
    m_pending = std::max(0, m_pending - 1);
    if (reply->error() != QNetworkReply::NoError) {
        if (m_error.isEmpty())
            m_error = tr("Could not load movie highlights: %1").arg(reply->errorString());
    } else {
        auto movies = parseMovies(reply->readAll(), shelf == Shelf::HighRatings);
        if (shelf == Shelf::Trending) m_trending.replaceMovies(std::move(movies));
        else if (shelf == Shelf::BestOfYear) m_bestOfYear.replaceMovies(std::move(movies));
        else m_highRatings.replaceMovies(std::move(movies));
    }
    reply->deleteLater();
    emit stateChanged();
}

void MovieHighlightsManager::cancelReplies()
{
    ++m_generation;
    for (const QPointer<QNetworkReply> &reply : std::as_const(m_replies)) {
        if (reply) {
            reply->abort();
            reply->deleteLater();
        }
    }
    m_replies.clear();
    m_pending = 0;
}
