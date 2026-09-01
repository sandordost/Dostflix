#include "app/AppPaths.h"
#include "app/AppSettings.h"
#include "library/LibraryDatabase.h"
#include "library/DownloadManager.h"
#include "library/LibraryMetadataManager.h"
#include "library/LibraryManager.h"
#include "movies/MovieListModel.h"
#include "network/NetworkGuardClient.h"
#include "network/SystemdScope.h"
#include "player/MpvPlayer.h"
#include "providers/ProviderManager.h"
#include "providers/ProwlarrManager.h"
#include "providers/SecretStore.h"
#include "streaming/TorrServerManager.h"
#include "subtitles/OpenSubtitlesManager.h"
#include "ui/AppController.h"
#include "vpn/NetworkManagerBackend.h"
#include "vpn/VpnManager.h"

#include <QDir>
#include <QGuiApplication>
#include <QFont>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QQmlApplicationEngine>
#include <QVariant>
#include <clocale>

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QGuiApplication app(argc, argv);
    QFont interfaceFont(QStringLiteral("Noto Sans"));
    interfaceFont.setPixelSize(14);
    app.setFont(interfaceFont);
    std::setlocale(LC_NUMERIC, "C");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
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
    LibraryDatabase libraryDatabase(
        QDir(paths.dataDir()).filePath(QStringLiteral("library.sqlite")),
        QStringLiteral("dostflix-library"));
    if (!libraryDatabase.open()) {
        qCritical().noquote() << "Could not open the library database:"
                              << libraryDatabase.lastError();
        return EXIT_FAILURE;
    }
    LibraryManager libraryManager(
        settings, libraryDatabase,
        QDir(paths.dataDir()).filePath(QStringLiteral("library")));
    DownloadManager downloadManager(libraryDatabase, libraryManager);
    NetworkManagerBackend vpnBackend;
    NetworkGuardClient networkGuard;
    VpnManager vpnManager(settings, vpnBackend, &networkGuard);
    LibSecretStore secretStore;
    ProviderManager providerManager(settings, secretStore);
    LibraryMetadataManager metadataManager(
        libraryDatabase, libraryManager, providerManager, paths.dataDir());
    OpenSubtitlesManager subtitleManager(settings, secretStore, paths.dataDir());

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
        QDir(paths.dataDir()).filePath(QStringLiteral("prowlarr")), movies, providerManager);
    TorrServerManager torrentEngine(
        QDir(paths.dataDir()).filePath(QStringLiteral("torrserver")));
    QObject::connect(&vpnManager, &VpnManager::stateChanged, &prowlarrManager,
                     [&] { prowlarrManager.setNetworkReady(vpnManager.networkReady()); });
    QObject::connect(&vpnManager, &VpnManager::stateChanged, &torrentEngine,
                     [&] { torrentEngine.setNetworkReady(vpnManager.networkReady()); });
    QObject::connect(&vpnManager, &VpnManager::stateChanged, &downloadManager,
                     [&] { downloadManager.setNetworkReady(vpnManager.networkReady()); });
    QObject::connect(&vpnManager, &VpnManager::stateChanged, &subtitleManager,
                     [&] { subtitleManager.setNetworkReady(vpnManager.networkReady()); });
    QObject::connect(&vpnManager, &VpnManager::stateChanged, &metadataManager,
                     [&] { metadataManager.setNetworkReady(vpnManager.networkReady()); });
    QObject::connect(&providerManager, &ProviderManager::tmdbTokenChanged,
                     &metadataManager, &LibraryMetadataManager::refresh);
    QObject::connect(&libraryManager, &LibraryManager::stateChanged,
                     &metadataManager, &LibraryMetadataManager::refresh,
                     Qt::QueuedConnection);
    QObject::connect(&libraryManager, &LibraryManager::subtitleContextChanged,
                     &subtitleManager, &OpenSubtitlesManager::setMediaContext);
    QObject::connect(&prowlarrManager, &ProwlarrManager::releasePrepared,
                     &torrentEngine,
                     [&](const QString &title, const QString &magnetUrl,
                         const QByteArray &torrentData) {
        if (!magnetUrl.isEmpty()) {
            if (!downloadManager.playMatchingRelease(title, magnetUrl))
                torrentEngine.startMagnet(title, magnetUrl);
        }
        else torrentEngine.startTorrentData(title, torrentData);
    });
    QObject::connect(&torrentEngine, &TorrServerManager::retentionSourceReady,
                     &downloadManager, &DownloadManager::beginTransfer);
    QObject::connect(&downloadManager, &DownloadManager::resumeRequested,
                     &torrentEngine, &TorrServerManager::resumeStoredTorrent);
    QObject::connect(&downloadManager, &DownloadManager::torrentPlaybackRequested,
                     &torrentEngine, &TorrServerManager::resumeStoredTorrent);
    QObject::connect(&downloadManager, &DownloadManager::torrentRemovalRequested,
                     &torrentEngine, &TorrServerManager::removeStoredTorrent);
    QObject::connect(&torrentEngine, &TorrServerManager::stateChanged,
                     &downloadManager, [&] {
        if (!torrentEngine.active() && downloadManager.active()) downloadManager.pause();
    });
    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("appController"), QVariant::fromValue(&controller)},
        {QStringLiteral("movieModel"), QVariant::fromValue(&movies)},
        {QStringLiteral("libraryManager"), QVariant::fromValue(&libraryManager)},
        {QStringLiteral("metadataManager"), QVariant::fromValue(&metadataManager)},
        {QStringLiteral("downloadManager"), QVariant::fromValue(&downloadManager)},
        {QStringLiteral("vpnManager"), QVariant::fromValue(&vpnManager)},
        {QStringLiteral("providerManager"), QVariant::fromValue(&providerManager)},
        {QStringLiteral("prowlarrManager"), QVariant::fromValue(&prowlarrManager)},
        {QStringLiteral("torrentEngine"), QVariant::fromValue(&torrentEngine)},
        {QStringLiteral("subtitleManager"), QVariant::fromValue(&subtitleManager)},
    });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(EXIT_FAILURE); },
                     Qt::QueuedConnection);
    engine.loadFromModule(QStringLiteral("Dostflix"), QStringLiteral("Main"));
    MpvPlayer *player = engine.rootObjects().isEmpty()
        ? nullptr
        : engine.rootObjects().constFirst()->findChild<MpvPlayer *>(QStringLiteral("videoPlayer"));
    if (player) {
        QObject::connect(player, &MpvPlayer::positionChanged, &libraryManager, [&] {
            libraryManager.recordPlaybackProgress(player->watchedSeconds(),
                                                   static_cast<int>(player->duration()));
        });
        QObject::connect(player, &MpvPlayer::durationChanged, &libraryManager, [&] {
            libraryManager.recordPlaybackProgress(player->watchedSeconds(),
                                                   static_cast<int>(player->duration()));
        });
        QObject::connect(player, &MpvPlayer::playbackStopping, &libraryManager,
                         [&](double position, double duration) {
            libraryManager.recordPlaybackProgress(static_cast<int>(position),
                                                   static_cast<int>(duration), true);
        });
    }
    QObject::connect(&subtitleManager, &OpenSubtitlesManager::subtitleReady,
                     &app, [player](const QUrl &url) { if (player) player->addSubtitleFile(url); });
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&, player] {
        if (player) player->stop();
        subtitleManager.cancel();
        metadataManager.cancel();
        downloadManager.pause();
        torrentEngine.shutdown();
        prowlarrManager.shutdown();
        vpnManager.shutdown();
    });
    vpnManager.start();
    return app.exec();
}
