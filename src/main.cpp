#include "movies/MovieListModel.h"
#include "ui/AppController.h"

#include <QGuiApplication>
#include <QQuickStyle>
#include <QQmlApplicationEngine>
#include <QVariant>

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("SandorDost"));
    QCoreApplication::setApplicationName(QStringLiteral("Dostflix"));

    AppController controller;
    MovieListModel movies;
    movies.replaceMovies({
        {"m1", "Arrival", 2016, {}, "4K", 128, 14'200'000'000LL},
        {"m2", "Moon", 2009, {}, "1080p", 84, 3'800'000'000LL},
        {"m3", "Metropolis", 1927, {}, "1080p", 61, 2'600'000'000LL},
        {"m4", "Stalker", 1979, {}, "4K", 43, 18'400'000'000LL},
        {"m5", "Solaris", 1972, {}, "1080p", 39, 6'100'000'000LL},
    });

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("appController"), QVariant::fromValue(&controller)},
        {QStringLiteral("movieModel"), QVariant::fromValue(&movies)},
    });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(EXIT_FAILURE); },
                     Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("Dostflix"), QStringLiteral("Main"));
    return app.exec();
}
