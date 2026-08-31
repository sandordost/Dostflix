#include "movies/MovieListModel.h"
#include <QtTest>

class MovieListModelTest final : public QObject
{
    Q_OBJECT

private slots:
    void exposesStableQmlRoles()
    {
        MovieListModel model;
        model.replaceMovies({Movie{QStringLiteral("m1"), QStringLiteral("Arrival"), 2016,
                                   QString(), QStringLiteral("1080p"), 42, 8'000'000'000LL,
                                   QString(), QString(), QString()}});
        QCOMPARE(model.rowCount(), 1);
        const QModelIndex first = model.index(0, 0);
        QCOMPARE(model.data(first, MovieListModel::TitleRole).toString(), QStringLiteral("Arrival"));
        QCOMPARE(model.data(first, MovieListModel::SeederCountRole).toInt(), 42);
        QCOMPARE(model.roleNames().value(MovieListModel::PosterUrlRole), QByteArray("posterUrl"));
    }

    void ignoresChildRowsAndInvalidIndexes()
    {
        MovieListModel model;
        model.replaceMovies({Movie{QStringLiteral("m1"), QStringLiteral("Arrival"), 2016,
                                   QString(), QStringLiteral("1080p"), 42, 8'000'000'000LL,
                                   QString(), QString(), QString()}});
        QCOMPARE(model.rowCount(model.index(0, 0)), 0);
        QVERIFY(!model.data(QModelIndex(), MovieListModel::TitleRole).isValid());
    }

    void matchesPostersByTitleAndYear()
    {
        MovieListModel model;
        model.replaceMovies({Movie{QStringLiteral("m1"), QStringLiteral("The.Matrix.1999.1080p"),
                                   1999, QString(), QStringLiteral("1080p"), 42, 1,
                                   QString(), QString(), QString()}});
        model.applyPosterMatches({{QStringLiteral("The Matrix"), 1999,
                                   QStringLiteral("https://image.test/matrix.jpg")}});
        QCOMPARE(model.data(model.index(0, 0), MovieListModel::PosterUrlRole).toString(),
                 QStringLiteral("https://image.test/matrix.jpg"));
    }
};

QTEST_GUILESS_MAIN(MovieListModelTest)
#include "tst_movie_list_model.moc"
