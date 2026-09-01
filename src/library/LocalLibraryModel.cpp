#include "library/LocalLibraryModel.h"

#include <QUrl>
#include <utility>

LocalLibraryModel::LocalLibraryModel(QObject *parent) : QAbstractListModel(parent) {}

int LocalLibraryModel::rowCount(const QModelIndex &parent) const
{ return parent.isValid() ? 0 : static_cast<int>(m_movies.size()); }

QVariant LocalLibraryModel::data(const QModelIndex &index, int role) const
{
    const LibraryMovie *movie = at(index.row());
    if (!index.isValid() || !movie) return {};
    switch (role) {
    case MovieIdRole: return movie->id;
    case TitleRole: return movie->title;
    case YearRole: return movie->year;
    case PosterUrlRole: return movie->posterPath.isEmpty()
        ? QVariant{} : QVariant::fromValue(QUrl::fromLocalFile(movie->posterPath));
    case VideoUrlRole: return QUrl::fromLocalFile(movie->videoPath);
    case WatchedSecondsRole: return movie->watchedSeconds;
    case DurationSecondsRole: return movie->durationSeconds;
    case SynopsisRole: return movie->synopsis;
    default: return {};
    }
}

QHash<int, QByteArray> LocalLibraryModel::roleNames() const
{
    return {{MovieIdRole, "movieId"}, {TitleRole, "title"}, {YearRole, "year"},
            {PosterUrlRole, "posterUrl"}, {VideoUrlRole, "videoUrl"},
            {WatchedSecondsRole, "watchedSeconds"},
            {DurationSecondsRole, "durationSeconds"}, {SynopsisRole, "synopsis"}};
}

void LocalLibraryModel::replace(QList<LibraryMovie> movies)
{
    beginResetModel();
    m_movies = std::move(movies);
    endResetModel();
}

void LocalLibraryModel::updateProgress(const QString &videoPath, int watchedSeconds,
                                       int durationSeconds)
{
    for (int row = 0; row < m_movies.size(); ++row) {
        LibraryMovie &movie = m_movies[row];
        if (movie.videoPath != videoPath) continue;
        movie.watchedSeconds = watchedSeconds;
        if (durationSeconds > 0) movie.durationSeconds = durationSeconds;
        const QModelIndex changed = index(row, 0);
        emit dataChanged(changed, changed, {WatchedSecondsRole, DurationSecondsRole});
        return;
    }
}

const LibraryMovie *LocalLibraryModel::at(int row) const
{
    return row >= 0 && row < m_movies.size() ? &m_movies.at(row) : nullptr;
}
