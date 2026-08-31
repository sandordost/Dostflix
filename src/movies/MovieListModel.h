#pragma once

#include "movies/Movie.h"

#include <QAbstractListModel>
#include <vector>

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
        SizeBytesRole
    };
    Q_ENUM(Role)

    explicit MovieListModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void replaceMovies(std::vector<Movie> movies);

private:
    std::vector<Movie> m_movies;
};
