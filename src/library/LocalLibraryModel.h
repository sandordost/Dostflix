#pragma once

#include "library/LibraryDatabase.h"

#include <QAbstractListModel>

class LocalLibraryModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        MovieIdRole = Qt::UserRole + 1,
        TitleRole,
        YearRole,
        PosterUrlRole,
        VideoUrlRole,
        WatchedSecondsRole,
        DurationSecondsRole
    };
    Q_ENUM(Role)

    explicit LocalLibraryModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void replace(QList<LibraryMovie> movies);
    const LibraryMovie *at(int row) const;

private:
    QList<LibraryMovie> m_movies;
};
