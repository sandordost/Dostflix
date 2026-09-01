#pragma once

#include "movies/Movie.h"

#include <QAbstractListModel>
#include <vector>

struct MoviePosterMatch final
{
    QString title;
    int year = 0;
    QString posterUrl;
};

class MovieListModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        YearRole,
        PosterUrlRole,
        QualityRole,
        SeederCountRole,
        SizeBytesRole,
        SourceLabelRole,
        DownloadUrlRole,
        MagnetUrlRole
    };
    Q_ENUM(Role)

    explicit MovieListModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void replaceMovies(std::vector<Movie> movies);
    void applyPosterMatches(const std::vector<MoviePosterMatch> &matches);

private:
    std::vector<Movie> m_movies;
};
