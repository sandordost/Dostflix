#pragma once

#include "movies/MovieListModel.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QUrl>

class ProviderManager;
class QNetworkReply;
class QUrlQuery;

class MovieHighlightsManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MovieListModel *trendingModel READ trendingModel CONSTANT)
    Q_PROPERTY(MovieListModel *bestOfYearModel READ bestOfYearModel CONSTANT)
    Q_PROPERTY(MovieListModel *highRatingsModel READ highRatingsModel CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool configured READ configured NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(int bestOfYear READ bestOfYear CONSTANT)

public:
    explicit MovieHighlightsManager(ProviderManager &providers,
                                    QUrl apiBase = QUrl(QStringLiteral("https://api.themoviedb.org/3/")),
                                    QObject *parent = nullptr);

    MovieListModel *trendingModel();
    MovieListModel *bestOfYearModel();
    MovieListModel *highRatingsModel();
    bool busy() const;
    bool configured() const;
    QString errorMessage() const;
    int bestOfYear() const;

    void setNetworkReady(bool ready);
    Q_INVOKABLE void refresh();

signals:
    void stateChanged();

private:
    enum class Shelf { Trending, BestOfYear, HighRatings };
    void request(Shelf shelf, const QString &path, const QUrlQuery &query);
    void complete(Shelf shelf, QNetworkReply *reply);
    void cancelReplies();

    ProviderManager &m_providers;
    QUrl m_apiBase;
    QNetworkAccessManager m_network;
    MovieListModel m_trending;
    MovieListModel m_bestOfYear;
    MovieListModel m_highRatings;
    QList<QPointer<QNetworkReply>> m_replies;
    QString m_error;
    int m_pending = 0;
    int m_generation = 0;
    int m_year = 0;
    bool m_networkReady = false;
};
