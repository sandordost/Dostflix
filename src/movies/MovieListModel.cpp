#include "movies/MovieListModel.h"

#include <utility>

MovieListModel::MovieListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MovieListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_movies.size());
}

QVariant MovieListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const Movie &movie = m_movies.at(static_cast<std::size_t>(index.row()));
    switch (role) {
    case IdRole:
        return movie.id;
    case TitleRole:
        return movie.title;
    case YearRole:
        return movie.year;
    case PosterUrlRole:
        return movie.posterUrl;
    case QualityRole:
        return movie.quality;
    case SeederCountRole:
        return movie.seederCount;
    case SizeBytesRole:
        return movie.sizeBytes;
    default:
        return {};
    }
}

QHash<int, QByteArray> MovieListModel::roleNames() const
{
    return {
        {IdRole, "movieId"},
        {TitleRole, "title"},
        {YearRole, "year"},
        {PosterUrlRole, "posterUrl"},
        {QualityRole, "quality"},
        {SeederCountRole, "seederCount"},
        {SizeBytesRole, "sizeBytes"},
    };
}

void MovieListModel::replaceMovies(std::vector<Movie> movies)
{
    beginResetModel();
    m_movies = std::move(movies);
    endResetModel();
}
