#include "providers/ProwlarrManager.h"

#include "movies/MovieListModel.h"
#include "providers/ProviderManager.h"
#include "providers/ReleaseResolver.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTcpSocket>
#include <QUuid>
#include <QUrl>
#include <QUrlQuery>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <csignal>
#include <sys/prctl.h>
#include <unistd.h>

ProwlarrManager::ProwlarrManager(QString dataDir, MovieListModel &movieModel,
                                 ProviderManager &providerManager, QObject *parent)
    : QObject(parent)
    , m_dataDir(std::move(dataDir))
    , m_movieModel(movieModel)
    , m_providerManager(providerManager)
{
    m_probeTimer.setInterval(500);
    connect(&m_probeTimer, &QTimer::timeout, this, &ProwlarrManager::probe);
    connect(&m_process, &QProcess::started, this, [this] {
        m_probeTimer.start();
        emit stateChanged();
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::Crashed || m_networkReady) {
            setError(m_process.errorString());
        }
    });
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
        m_probeTimer.stop();
        m_ready = false;
        if (m_networkReady && exitCode != 0) {
            setError(tr("Prowlarr stopped unexpectedly (code %1)").arg(exitCode));
        } else {
            emit stateChanged();
        }
    });
}

bool ProwlarrManager::installed() const { return QFile::exists(QString::fromLatin1(Executable)); }
bool ProwlarrManager::running() const { return m_process.state() != QProcess::NotRunning; }
bool ProwlarrManager::ready() const { return m_ready; }
QString ProwlarrManager::errorMessage() const { return m_error; }
QString ProwlarrManager::webUrl() const { return QStringLiteral("http://127.0.0.1:9696"); }
QString ProwlarrManager::apiKey() const { return m_apiKey; }
bool ProwlarrManager::searchBusy() const
{
    return !m_searchReply.isNull() || !m_metadataReply.isNull();
}
QString ProwlarrManager::searchError() const { return m_searchError; }
bool ProwlarrManager::releaseBusy() const { return !m_releaseReply.isNull(); }
QString ProwlarrManager::releaseError() const { return m_releaseError; }

QString ProwlarrManager::stateLabel() const
{
    if (!installed()) return tr("Prowlarr is not installed");
    if (!m_error.isEmpty()) return tr("Prowlarr error");
    if (m_ready) return tr("Prowlarr ready");
    if (running()) return tr("Prowlarr starting…");
    return m_networkReady ? tr("Prowlarr stopped") : tr("Waiting for VPN");
}

void ProwlarrManager::setNetworkReady(bool ready)
{
    if (m_networkReady == ready) return;
    m_networkReady = ready;
    ready ? start() : stop();
    emit stateChanged();
}

void ProwlarrManager::shutdown()
{
    m_networkReady = false;
    stop();
}

void ProwlarrManager::search(const QString &query)
{
    const QString trimmed = query.trimmed();
    if (!m_ready || trimmed.isEmpty()) return;

    if (m_searchReply) {
        QNetworkReply *oldReply = m_searchReply;
        m_searchReply = nullptr;
        oldReply->abort();
        oldReply->deleteLater();
    }
    if (m_metadataReply) {
        QNetworkReply *oldReply = m_metadataReply;
        m_metadataReply = nullptr;
        oldReply->abort();
        oldReply->deleteLater();
    }

    m_searchQuery = trimmed;
    m_searchError.clear();
    QUrl url(webUrl() + QStringLiteral("/api/v1/search"));
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("query"), trimmed);
    parameters.addQueryItem(QStringLiteral("type"), QStringLiteral("search"));
    parameters.addQueryItem(QStringLiteral("categories"), QStringLiteral("2000"));
    parameters.addQueryItem(QStringLiteral("limit"), QStringLiteral("100"));
    parameters.addQueryItem(QStringLiteral("offset"), QStringLiteral("0"));
    url.setQuery(parameters);
    QNetworkRequest request(url);
    request.setRawHeader("X-Api-Key", m_apiKey.toUtf8());
    request.setTransferTimeout(60'000);
    m_searchReply = m_network.get(request);
    emit searchStateChanged();

    QNetworkReply *searchReply = m_searchReply;
    connect(searchReply, &QNetworkReply::finished, this, [this, searchReply] {
        if (m_searchReply != searchReply) return;
        QNetworkReply *reply = searchReply;
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            m_searchError = reply->errorString();
        } else {
            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
                m_searchError = tr("Prowlarr returned an invalid response");
            } else {
                static const QRegularExpression yearPattern(
                    QStringLiteral("(?:^|[ .(])(19\\d{2}|20\\d{2})(?:[ .)]|$)"));
                std::vector<Movie> movies;
                const QJsonArray releases = document.array();
                movies.reserve(static_cast<std::size_t>(releases.size()));
                for (const QJsonValue &value : releases) {
                    const QJsonObject item = value.toObject();
                    const QString title = item.value(QStringLiteral("title")).toString();
                    if (title.isEmpty()) continue;
                    const auto match = yearPattern.match(title);
                    QString quality;
                    const QStringList candidates = {QStringLiteral("2160p"), QStringLiteral("1080p"),
                                                    QStringLiteral("720p"), QStringLiteral("4K")};
                    for (const QString &candidate : candidates) {
                        if (title.contains(candidate, Qt::CaseInsensitive)) {
                            quality = candidate == QStringLiteral("2160p")
                                ? QStringLiteral("4K") : candidate;
                            break;
                        }
                    }
                    movies.push_back({item.value(QStringLiteral("guid")).toString(), title,
                        match.hasMatch() ? match.captured(1).toInt() : 0,
                        item.value(QStringLiteral("posterUrl")).toString(), quality,
                        item.value(QStringLiteral("seeders")).toInt(),
                        static_cast<qint64>(item.value(QStringLiteral("size")).toDouble()),
                        item.value(QStringLiteral("indexer")).toString(),
                        item.value(QStringLiteral("downloadUrl")).toString(),
                        item.value(QStringLiteral("magnetUrl")).toString()});
                }
                m_movieModel.replaceMovies(std::move(movies));
            }
        }
        reply->deleteLater();
        m_searchReply = nullptr;
        if (m_searchError.isEmpty() && m_movieModel.rowCount() > 0
            && m_providerManager.hasTmdbToken()) {
            fetchMetadata(m_searchQuery);
        }
        emit searchStateChanged();
    });
}

void ProwlarrManager::fetchMetadata(const QString &query)
{
    QUrl url(QStringLiteral("https://api.themoviedb.org/3/search/movie"));
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("query"), query);
    parameters.addQueryItem(QStringLiteral("include_adult"), QStringLiteral("false"));
    parameters.addQueryItem(QStringLiteral("language"), QStringLiteral("nl-NL"));
    parameters.addQueryItem(QStringLiteral("page"), QStringLiteral("1"));
    url.setQuery(parameters);
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Authorization",
                         QByteArray("Bearer ") + m_providerManager.tmdbToken().toUtf8());
    request.setTransferTimeout(15'000);
    m_metadataReply = m_network.get(request);
    QNetworkReply *metadataReply = m_metadataReply;
    emit searchStateChanged();

    connect(metadataReply, &QNetworkReply::finished, this, [this, metadataReply] {
        if (m_metadataReply != metadataReply) return;
        if (metadataReply->error() == QNetworkReply::NoError) {
            const QJsonDocument document = QJsonDocument::fromJson(metadataReply->readAll());
            const QJsonArray results = document.object().value(QStringLiteral("results")).toArray();
            std::vector<MoviePosterMatch> matches;
            matches.reserve(static_cast<std::size_t>(results.size()));
            for (const QJsonValue &value : results) {
                const QJsonObject movie = value.toObject();
                const QString path = movie.value(QStringLiteral("poster_path")).toString();
                const QString releaseDate = movie.value(QStringLiteral("release_date")).toString();
                if (!path.isEmpty()) {
                    const QString posterUrl =
                        QStringLiteral("https://image.tmdb.org/t/p/w500%1").arg(path);
                    const int year = releaseDate.left(4).toInt();
                    matches.push_back({movie.value(QStringLiteral("title")).toString(),
                                       year, posterUrl});
                    const QString originalTitle =
                        movie.value(QStringLiteral("original_title")).toString();
                    if (originalTitle != matches.back().title) {
                        matches.push_back({originalTitle, year, posterUrl});
                    }
                }
            }
            m_movieModel.applyPosterMatches(matches);
        }
        metadataReply->deleteLater();
        m_metadataReply = nullptr;
        emit searchStateChanged();
    });
}

void ProwlarrManager::prepareRelease(const QString &title, const QString &magnetUrl,
                                     const QString &downloadUrl)
{
    if (!m_ready) return;
    if (m_releaseReply) {
        QNetworkReply *reply = m_releaseReply;
        m_releaseReply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
    m_releaseError.clear();
    const ReleaseLocation location = resolveReleaseLocation(magnetUrl, downloadUrl);
    if (!location.magnetUrl.isEmpty()) {
        emit releasePrepared(title, location.magnetUrl, {});
        emit releaseStateChanged();
        return;
    }
    if (location.torrentUrl.isEmpty()) {
        m_releaseError = tr("This release has no usable magnet or torrent link");
        emit releaseStateChanged();
        return;
    }
    fetchRelease(title, location.torrentUrl, 5);
}

void ProwlarrManager::fetchRelease(const QString &title, const QUrl &url,
                                   int redirectsRemaining)
{
    QNetworkRequest request(url);
    const QUrl prowlarrUrl(webUrl());
    if (url.host() == prowlarrUrl.host() && url.port() == prowlarrUrl.port()) {
        request.setRawHeader("X-Api-Key", m_apiKey.toUtf8());
    }
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(30'000);
    m_releaseReply = m_network.get(request);
    QNetworkReply *releaseReply = m_releaseReply;
    emit releaseStateChanged();
    connect(releaseReply, &QNetworkReply::finished, this,
            [this, releaseReply, title, redirectsRemaining] {
        if (m_releaseReply != releaseReply) return;
        const QByteArray torrentData = releaseReply->readAll();
        const QString returnedText = QString::fromUtf8(torrentData).trimmed();
        QUrl redirect = releaseReply->attribute(
            QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (redirect.isRelative()) redirect = releaseReply->url().resolved(redirect);
        if (isMagnetUrl(redirect.toString())) {
            emit releasePrepared(title, redirect.toString(), {});
        } else if (!redirect.isEmpty() && redirectsRemaining > 0) {
            const ReleaseLocation next = resolveReleaseLocation({}, redirect.toString());
            if (next.torrentUrl.isEmpty()) {
                m_releaseError = tr("Prowlarr redirected to an unsupported release link");
            } else {
                releaseReply->deleteLater();
                m_releaseReply = nullptr;
                fetchRelease(title, next.torrentUrl, redirectsRemaining - 1);
                return;
            }
        } else if (!redirect.isEmpty()) {
            m_releaseError = tr("Too many redirects while retrieving the torrent");
        } else if (isMagnetUrl(returnedText)) {
            emit releasePrepared(title, returnedText, {});
        } else if (releaseReply->error() != QNetworkReply::NoError || torrentData.isEmpty()) {
            m_releaseError = releaseReply->error() == QNetworkReply::NoError
                ? tr("Prowlarr returned an empty torrent file")
                : releaseReply->errorString();
        } else {
            emit releasePrepared(title, {}, torrentData);
        }
        releaseReply->deleteLater();
        m_releaseReply = nullptr;
        emit releaseStateChanged();
    });
}

void ProwlarrManager::openWebInterface()
{
    if (m_ready) QDesktopServices::openUrl(QUrl(webUrl()));
}

bool ProwlarrManager::ensureConfig()
{
    if (!QDir().mkpath(m_dataDir)) return false;
    const QString path = QDir(m_dataDir).filePath(QStringLiteral("config.xml"));
    QFile existing(path);
    if (existing.open(QIODevice::ReadOnly)) {
        QByteArray contents = existing.readAll();
        existing.close();
        QXmlStreamReader xml(contents);
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == QStringLiteral("ApiKey")) {
                m_apiKey = xml.readElementText();
                break;
            }
        }
        if (!m_apiKey.isEmpty()) {
            const QRegularExpression bindPattern(
                QStringLiteral("<BindAddress>[^<]*</BindAddress>"));
            QString safeContents = QString::fromUtf8(contents);
            safeContents.replace(bindPattern,
                                 QStringLiteral("<BindAddress>127.0.0.1</BindAddress>"));
            if (safeContents.toUtf8() == contents) return true;
            QSaveFile safeFile(path);
            if (!safeFile.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
            safeFile.write(safeContents.toUtf8());
            return safeFile.commit();
        }
    }

    m_apiKey = QUuid::createUuid().toString(QUuid::WithoutBraces).remove(u'-');
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(QStringLiteral("Config"));
    const auto field = [&xml](const QString &name, const QString &value) {
        xml.writeTextElement(name, value);
    };
    field(QStringLiteral("BindAddress"), QStringLiteral("127.0.0.1"));
    field(QStringLiteral("Port"), QStringLiteral("9696"));
    field(QStringLiteral("EnableSsl"), QStringLiteral("False"));
    field(QStringLiteral("LaunchBrowser"), QStringLiteral("False"));
    field(QStringLiteral("ApiKey"), m_apiKey);
    field(QStringLiteral("AuthenticationMethod"), QStringLiteral("Forms"));
    field(QStringLiteral("AuthenticationRequired"), QStringLiteral("DisabledForLocalAddresses"));
    field(QStringLiteral("Branch"), QStringLiteral("master"));
    field(QStringLiteral("LogLevel"), QStringLiteral("info"));
    field(QStringLiteral("UrlBase"), QString());
    field(QStringLiteral("InstanceName"), QStringLiteral("Dostflix Prowlarr"));
    field(QStringLiteral("UpdateMechanism"), QStringLiteral("External"));
    xml.writeEndElement();
    xml.writeEndDocument();
    return file.commit();
}

void ProwlarrManager::start()
{
    if (running() || !installed()) return;
    m_error.clear();
    if (!ensureConfig()) {
        setError(tr("Could not create the Prowlarr data directory"));
        return;
    }
    m_process.setProgram(QString::fromLatin1(Executable));
    m_process.setArguments({QStringLiteral("-nobrowser"),
                            QStringLiteral("-data=%1").arg(m_dataDir)});
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    m_process.setChildProcessModifier([] {
        ::prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (::getppid() == 1) ::_exit(EXIT_FAILURE);
    });
    m_process.start();
}

void ProwlarrManager::stop()
{
    if (m_searchReply) {
        QNetworkReply *reply = m_searchReply;
        m_searchReply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
    if (m_metadataReply) {
        QNetworkReply *reply = m_metadataReply;
        m_metadataReply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
    if (m_releaseReply) {
        QNetworkReply *reply = m_releaseReply;
        m_releaseReply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
    emit searchStateChanged();
    emit releaseStateChanged();
    m_probeTimer.stop();
    m_ready = false;
    if (!running()) return;
    m_process.terminate();
    if (!m_process.waitForFinished(5'000)) {
        m_process.kill();
        m_process.waitForFinished(2'000);
    }
    emit stateChanged();
}

void ProwlarrManager::probe()
{
    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, 9696);
    if (!socket.waitForConnected(100)) return;
    socket.disconnectFromHost();
    m_probeTimer.stop();
    if (!m_ready) {
        m_ready = true;
        emit stateChanged();
    }
}

void ProwlarrManager::setError(QString error)
{
    m_error = std::move(error);
    emit stateChanged();
}
