#include "app/AppPaths.h"
#include "app/AppSettings.h"
#include "movies/MovieListModel.h"
#include "network/NetworkGuardClient.h"
#include "network/SystemdScope.h"
#include "providers/ProviderManager.h"
#include "providers/ProwlarrManager.h"
#include "providers/SecretStore.h"
#include "ui/AppController.h"
#include "vpn/NetworkManagerBackend.h"
#include "vpn/VpnManager.h"

#include <QDir>
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

    QString scopeError;
    if (!SystemdScope::enter(&scopeError)) {
        qWarning().noquote() << scopeError;
    }

    const AppPaths paths;
    if (!paths.ensureExists()) {
        return EXIT_FAILURE;
    }
    AppSettings settings(QDir(paths.configDir()).filePath(QStringLiteral("settings.ini")));
    NetworkManagerBackend vpnBackend;
    NetworkGuardClient networkGuard;
    VpnManager vpnManager(settings, vpnBackend, &networkGuard);
    LibSecretStore secretStore;
    ProviderManager providerManager(settings, secretStore);

    AppController controller;
    MovieListModel movies;
    movies.replaceMovies({
        {"m1", "Arrival", 2016, {}, "4K", 128, 14'200'000'000LL, {}, {}, {}},
        {"m2", "Moon", 2009, {}, "1080p", 84, 3'800'000'000LL, {}, {}, {}},
        {"m3", "Metropolis", 1927, {}, "1080p", 61, 2'600'000'000LL, {}, {}, {}},
        {"m4", "Stalker", 1979, {}, "4K", 43, 18'400'000'000LL, {}, {}, {}},
        {"m5", "Solaris", 1972, {}, "1080p", 39, 6'100'000'000LL, {}, {}, {}},
    });
    ProwlarrManager prowlarrManager(
        QDir(paths.dataDir()).filePath(QStringLiteral("prowlarr")), movies);
    QObject::connect(&vpnManager, &VpnManager::stateChanged, &prowlarrManager,
                     [&] { prowlarrManager.setNetworkReady(vpnManager.networkReady()); });
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&] {
        prowlarrManager.shutdown();
        vpnManager.shutdown();
    });

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("appController"), QVariant::fromValue(&controller)},
        {QStringLiteral("movieModel"), QVariant::fromValue(&movies)},
        {QStringLiteral("vpnManager"), QVariant::fromValue(&vpnManager)},
        {QStringLiteral("providerManager"), QVariant::fromValue(&providerManager)},
        {QStringLiteral("prowlarrManager"), QVariant::fromValue(&prowlarrManager)},
    });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(EXIT_FAILURE); },
                     Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("Dostflix"), QStringLiteral("Main"));
    vpnManager.start();
    return app.exec();
}
