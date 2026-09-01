#include "streaming/TorrServerManager.h"

#include "streaming/BufferController.h"

#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QTcpServer>
#include <utility>

namespace {
bool successful(const QNetworkReply *reply)
{
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return reply->error() == QNetworkReply::NoError && status >= 200 && status < 300;
}

bool isVideoFile(const QString &path, qint64 size)
{
    static const QStringList extensions = {
        QStringLiteral("mkv"), QStringLiteral("mp4"), QStringLiteral("webm"),
        QStringLiteral("avi"), QStringLiteral("mov"), QStringLiteral("m4v"),
        QStringLiteral("ts")};
    return size >= 50LL * 1024 * 1024
        && extensions.contains(QFileInfo(path).suffix().toLower());
}

QString replyError(QNetworkReply *reply)
{
    const QString body = QString::fromUtf8(reply->readAll()).trimmed();
    return body.isEmpty() ? reply->errorString() : body;
}
}

TorrServerManager::TorrServerManager(QString dataDir, QObject *parent)
    : QObject(parent), m_dataDir(std::move(dataDir))
{
    m_pollTimer.setInterval(500);
    connect(&m_pollTimer, &QTimer::timeout, this, &TorrServerManager::poll);
    connect(&m_process, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
        m_daemonReady = false;
        if (!m_stopping && m_networkReady) fail(tr("The TorrServer streaming service stopped unexpectedly"));
    });
}

TorrServerManager::~TorrServerManager() { shutdown(); }
TorrentFileModel *TorrServerManager::videoFiles() { return &m_videoFiles; }
bool TorrServerManager::active() const { return m_active; }
bool TorrServerManager::backendReady() const { return m_daemonReady; }
bool TorrServerManager::needsFileSelection() const { return m_needsFileSelection; }
QString TorrServerManager::title() const { return m_title; }
QString TorrServerManager::selectedFileName() const { return m_selectedFileName; }
QString TorrServerManager::stateLabel() const { return m_stateLabel; }
QString TorrServerManager::errorMessage() const { return m_error; }
double TorrServerManager::progress() const { return m_progress; }
qint64 TorrServerManager::downloadRate() const { return m_downloadRate; }
int TorrServerManager::peerCount() const { return m_peerCount; }
int TorrServerManager::seedCount() const { return m_seedCount; }
double TorrServerManager::distributedCopies() const { return 0.0; }
double TorrServerManager::bufferSeconds() const { return m_bufferSeconds; }
double TorrServerManager::estimatedWaitSeconds() const { return m_estimatedWaitSeconds; }
bool TorrServerManager::bufferReady() const { return m_bufferReady; }

QString TorrServerManager::streamUrl() const
{
    if (m_hash.isEmpty() || m_selectedFileId < 0) return {};
    QUrl url = m_baseUrl.resolved(QUrl(QStringLiteral("stream/video")));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("link"), m_hash);
    query.addQueryItem(QStringLiteral("index"), QString::number(m_selectedFileId));
    query.addQueryItem(QStringLiteral("play"), QString());
    url.setQuery(query);
    return url.toString();
}

void TorrServerManager::setNetworkReady(bool ready)
{
    if (m_networkReady == ready) return;
    m_networkReady = ready;
    if (ready) startDaemon();
    else {
        const bool hadTransfer = m_active;
        stopDaemon(true);
        clearTransferState();
        if (hadTransfer) {
            m_error = tr("Stream stopped because VPN protection was lost");
            m_stateLabel = tr("VPN required");
            emit stateChanged();
        }
    }
}

void TorrServerManager::startDaemon()
{
    if (!m_networkReady || m_process.state() != QProcess::NotRunning) return;
    if (!QFileInfo::exists(QStringLiteral("/usr/bin/torrserver"))) {
        fail(tr("TorrServer is not installed"));
        return;
    }
    if (!QDir().mkpath(m_dataDir)) {
        fail(tr("Could not create the TorrServer data directory"));
        return;
    }
    QTcpServer portProbe;
    if (!portProbe.listen(QHostAddress::LocalHost, 0)) {
        fail(tr("Could not reserve a local TorrServer API port"));
        return;
    }
    const quint16 apiPort = portProbe.serverPort();
    portProbe.close();
    const quint16 peerPort = static_cast<quint16>(QRandomGenerator::global()->bounded(20'000, 60'000));
    m_baseUrl = QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(apiPort));
    m_stopping = false;
    m_daemonReady = false;
    m_process.setProgram(QStringLiteral("/usr/bin/torrserver"));
    m_process.setArguments({QStringLiteral("--ip"), QStringLiteral("127.0.0.1"),
                            QStringLiteral("--port"), QString::number(apiPort),
                            QStringLiteral("--path"), m_dataDir,
                            QStringLiteral("--torrentaddr"),
                            QStringLiteral(":%1").arg(peerPort)});
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    m_process.start();
    m_pollTimer.start();
    if (!m_active) m_stateLabel = tr("Starting TorrServer…");
    emit stateChanged();
}

void TorrServerManager::stopDaemon(bool force)
{
    m_pollTimer.stop();
    if (m_reply) {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
    m_daemonReady = false;
    if (m_process.state() == QProcess::NotRunning) return;
    m_stopping = true;
    force ? m_process.kill() : m_process.terminate();
    if (!m_process.waitForFinished(force ? 1'000 : 3'000)) {
        m_process.kill();
        m_process.waitForFinished(1'000);
    }
    m_stopping = false;
}

void TorrServerManager::poll()
{
    if (!m_networkReady || m_reply) return;
    if (!m_daemonReady) probeApi();
    else if (m_active && m_hash.isEmpty()) submitPendingRelease();
    else if (m_active) requestStatus();
}

void TorrServerManager::probeApi()
{
    get(QStringLiteral("echo"), {}, [this](QNetworkReply *reply) {
        if (!successful(reply)) return;
        m_daemonReady = true;
        emit stateChanged();
        submitPendingRelease();
    });
}

void TorrServerManager::startMagnet(const QString &title, const QString &magnetUrl)
{
    if (!m_networkReady) { fail(tr("VPN protection must be ready before starting a torrent")); return; }
    if (!magnetUrl.startsWith(QStringLiteral("magnet:?"), Qt::CaseInsensitive)) {
        fail(tr("This release does not contain a valid magnet link")); return;
    }
    clearTransferState();
    m_title = title;
    m_pendingMagnet = magnetUrl;
    m_active = true;
    m_stateLabel = tr("Submitting stream to TorrServer…");
    emit stateChanged();
    startDaemon();
    if (m_daemonReady) submitPendingRelease();
}

void TorrServerManager::startTorrentData(const QString &title, const QByteArray &torrentData)
{
    if (!m_networkReady) { fail(tr("VPN protection must be ready before starting a torrent")); return; }
    if (torrentData.isEmpty()) { fail(tr("The torrent file is empty")); return; }
    clearTransferState();
    m_title = title;
    m_pendingTorrent = torrentData;
    m_active = true;
    m_stateLabel = tr("Submitting stream to TorrServer…");
    emit stateChanged();
    startDaemon();
    if (m_daemonReady) submitPendingRelease();
}

void TorrServerManager::submitPendingRelease()
{
    if (!m_active || !m_daemonReady || m_reply || !m_hash.isEmpty()) return;
    if (!m_pendingMagnet.isEmpty()) {
        QJsonObject body{{QStringLiteral("action"), QStringLiteral("add")},
                         {QStringLiteral("link"), m_pendingMagnet},
                         {QStringLiteral("title"), m_title},
                         {QStringLiteral("category"), QStringLiteral("movie")},
                         {QStringLiteral("save_to_db"), true}};
        postJson(QStringLiteral("torrents"), body, [this](QNetworkReply *reply) {
            if (!successful(reply)) { fail(replyError(reply)); return; }
            m_pendingMagnet.clear();
            parseStatus(reply->readAll());
        });
        return;
    }
    auto *multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart file;
    file.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QStringLiteral("form-data; name=\"file\"; filename=\"release.torrent\""));
    file.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-bittorrent"));
    file.setBody(m_pendingTorrent);
    multi->append(file);
    QHttpPart save;
    save.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QStringLiteral("form-data; name=\"save\""));
    save.setBody("true");
    multi->append(save);
    QNetworkReply *reply = m_network.post(request(QStringLiteral("torrent/upload")), multi);
    multi->setParent(reply);
    m_reply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        if (!successful(reply)) fail(replyError(reply));
        else { m_pendingTorrent.clear(); parseStatus(reply->readAll()); }
        reply->deleteLater();
    });
}

void TorrServerManager::requestStatus()
{
    postJson(QStringLiteral("torrents"),
             QJsonObject{{QStringLiteral("action"), QStringLiteral("get")},
                         {QStringLiteral("hash"), m_hash}},
             [this](QNetworkReply *reply) {
        if (successful(reply)) parseStatus(reply->readAll());
    });
}

void TorrServerManager::parseStatus(const QByteArray &body)
{
    const QJsonObject status = QJsonDocument::fromJson(body).object();
    const QString hash = status.value(QStringLiteral("hash")).toString();
    if (!hash.isEmpty()) m_hash = hash;
    if (m_hash.isEmpty()) { fail(tr("TorrServer did not return a torrent hash")); return; }
    m_downloadRate = static_cast<qint64>(status.value(QStringLiteral("download_speed")).toDouble());
    m_peerCount = status.value(QStringLiteral("active_peers")).toInt();
    m_seedCount = status.value(QStringLiteral("connected_seeders")).toInt();
    const qint64 total = static_cast<qint64>(status.value(QStringLiteral("torrent_size")).toDouble());
    const qint64 loaded = static_cast<qint64>(status.value(QStringLiteral("loaded_size")).toDouble());
    m_progress = total > 0
        ? qBound(0.0, static_cast<double>(loaded) / static_cast<double>(total), 1.0)
        : 0.0;

    std::vector<TorrentVideoFile> videos;
    for (const QJsonValue &value : status.value(QStringLiteral("file_stats")).toArray()) {
        const QJsonObject file = value.toObject();
        const int id = file.value(QStringLiteral("id")).toInt(-1);
        const QString path = file.value(QStringLiteral("path")).toString();
        const qint64 size = static_cast<qint64>(file.value(QStringLiteral("length")).toDouble());
        if (id >= 0 && isVideoFile(path, size)) videos.push_back({id, path, size});
    }
    if (m_videoFiles.rowCount() == 0 && !videos.empty()) {
        m_videoFiles.replace(videos);
        if (videos.size() == 1) {
            m_selectedFileId = videos.front().torrentIndex;
            m_selectedFileName = videos.front().path;
            m_selectedFileSize = videos.front().sizeBytes;
            startPreload();
        } else {
            m_needsFileSelection = true;
            m_stateLabel = tr("Choose a video file");
        }
    } else if (videos.empty()) {
        m_stateLabel = tr("Acquiring torrent metadata…");
    }

    if (m_preloadStarted && m_selectedFileSize > 0) {
        const qint64 preloaded = static_cast<qint64>(status.value(QStringLiteral("preloaded_bytes")).toDouble());
        const double duration = status.value(QStringLiteral("duration_seconds")).toDouble();
        const qint64 bitrate = duration > 0.0
            ? static_cast<qint64>((static_cast<double>(m_selectedFileSize) * 8.0) / duration)
            : 8'000'000;
        const BufferEstimate estimate = BufferController::estimate(
            preloaded, m_selectedFileSize, bitrate, m_downloadRate);
        m_bufferSeconds = estimate.playableSeconds;
        m_estimatedWaitSeconds = estimate.estimatedWaitSeconds;
        m_bufferReady = estimate.ready;
        m_stateLabel = m_bufferReady ? tr("Ready to play") : tr("Building a safe playback buffer…");
    }
    emit statisticsChanged();
    emit stateChanged();
}

void TorrServerManager::selectVideoFile(int row)
{
    const TorrentVideoFile *file = m_videoFiles.at(row);
    if (!file || !m_active) return;
    m_selectedFileId = file->torrentIndex;
    m_selectedFileName = file->path;
    m_selectedFileSize = file->sizeBytes;
    m_needsFileSelection = false;
    startPreload();
}

void TorrServerManager::startPreload()
{
    if (m_preloadStarted || m_hash.isEmpty() || m_selectedFileId < 0 || m_reply) return;
    m_preloadStarted = true;
    m_stateLabel = tr("Building a safe playback buffer…");
    emit stateChanged();
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("link"), m_hash);
    query.addQueryItem(QStringLiteral("index"), QString::number(m_selectedFileId));
    query.addQueryItem(QStringLiteral("preload"), QString());
    query.addQueryItem(QStringLiteral("stat"), QString());
    get(QStringLiteral("stream/video"), query, [this](QNetworkReply *reply) {
        if (!successful(reply)) fail(replyError(reply));
        else parseStatus(reply->readAll());
    });
}

void TorrServerManager::cancel()
{
    if (m_daemonReady && !m_hash.isEmpty() && !m_reply) {
        postJson(QStringLiteral("torrents"),
                 QJsonObject{{QStringLiteral("action"), QStringLiteral("drop")},
                             {QStringLiteral("hash"), m_hash}},
                 [](QNetworkReply *) {});
    }
    clearTransferState();
}

void TorrServerManager::shutdown()
{
    m_networkReady = false;
    stopDaemon(false);
    clearTransferState();
}

void TorrServerManager::clearTransferState()
{
    m_videoFiles.replace({});
    m_title.clear(); m_hash.clear(); m_pendingMagnet.clear(); m_pendingTorrent.clear();
    m_selectedFileName.clear(); m_stateLabel.clear(); m_error.clear();
    m_active = false; m_needsFileSelection = false; m_preloadStarted = false; m_bufferReady = false;
    m_selectedFileId = -1; m_selectedFileSize = 0; m_progress = 0.0; m_downloadRate = 0;
    m_peerCount = 0; m_seedCount = 0; m_bufferSeconds = 0.0; m_estimatedWaitSeconds = 0.0;
    emit stateChanged();
    emit statisticsChanged();
}

void TorrServerManager::fail(QString error)
{
    m_error = std::move(error);
    m_stateLabel = tr("Streaming error");
    m_active = false;
    emit stateChanged();
}

QNetworkRequest TorrServerManager::request(const QString &path) const
{
    QNetworkRequest result(m_baseUrl.resolved(QUrl(path)));
    result.setTransferTimeout(15'000);
    return result;
}

void TorrServerManager::get(const QString &path, const QUrlQuery &query,
                            std::function<void(QNetworkReply *)> finished)
{
    QUrl url = m_baseUrl.resolved(QUrl(path));
    url.setQuery(query);
    QNetworkReply *reply = m_network.get(QNetworkRequest(url));
    m_reply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, finished = std::move(finished)] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        finished(reply);
        reply->deleteLater();
    });
}

void TorrServerManager::postJson(const QString &path, const QJsonObject &body,
                                 std::function<void(QNetworkReply *)> finished)
{
    QNetworkRequest apiRequest = request(path);
    apiRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *reply = m_network.post(apiRequest, QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_reply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, finished = std::move(finished)] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        finished(reply);
        reply->deleteLater();
    });
}
