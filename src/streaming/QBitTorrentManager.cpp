#include "streaming/QBitTorrentManager.h"

#include "streaming/BufferController.h"

#include <QDir>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QHostAddress>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QSettings>
#include <QTcpServer>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <utility>

namespace {
bool isVideoFile(const QString &path, qint64 size)
{
    static const QStringList extensions = {
        QStringLiteral("mkv"), QStringLiteral("mp4"), QStringLiteral("webm"),
        QStringLiteral("avi"), QStringLiteral("mov"), QStringLiteral("m4v"),
        QStringLiteral("ts")};
    return size >= 50LL * 1024 * 1024
        && extensions.contains(QFileInfo(path).suffix().toLower());
}

QHttpPart textPart(const QByteArray &name, const QByteArray &value)
{
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   QStringLiteral("form-data; name=\"%1\"")
                       .arg(QString::fromLatin1(name)));
    part.setBody(value);
    return part;
}

bool successful(const QNetworkReply *reply)
{
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return reply->error() == QNetworkReply::NoError && status >= 200 && status < 300;
}

bool readBencodedString(const QByteArray &data, qsizetype &position, QByteArray *value)
{
    const qsizetype colon = data.indexOf(':', position);
    if (colon <= position) return false;
    bool valid = false;
    const qsizetype length = data.mid(position, colon - position).toLongLong(&valid);
    if (!valid || length < 0 || colon + 1 + length > data.size()) return false;
    position = colon + 1;
    if (value) *value = data.mid(position, length);
    position += length;
    return true;
}

bool skipBencodedValue(const QByteArray &data, qsizetype &position)
{
    if (position >= data.size()) return false;
    const char marker = data.at(position);
    if (marker >= '0' && marker <= '9') return readBencodedString(data, position, nullptr);
    if (marker == 'i') {
        const qsizetype end = data.indexOf('e', ++position);
        if (end < 0) return false;
        position = end + 1;
        return true;
    }
    if (marker != 'l' && marker != 'd') return false;
    ++position;
    while (position < data.size() && data.at(position) != 'e') {
        if (!skipBencodedValue(data, position)) return false;
    }
    if (position >= data.size()) return false;
    ++position;
    return true;
}

QString torrentInfoHash(const QByteArray &torrentData)
{
    if (torrentData.isEmpty() || torrentData.at(0) != 'd') return {};
    qsizetype position = 1;
    while (position < torrentData.size() && torrentData.at(position) != 'e') {
        QByteArray key;
        if (!readBencodedString(torrentData, position, &key)) return {};
        const qsizetype valueStart = position;
        if (!skipBencodedValue(torrentData, position)) return {};
        if (key == QByteArrayLiteral("info")) {
            return QString::fromLatin1(QCryptographicHash::hash(
                torrentData.mid(valueStart, position - valueStart),
                QCryptographicHash::Sha1).toHex());
        }
    }
    return {};
}

QString magnetInfoHash(const QString &magnetUrl)
{
    const QString xt = QUrlQuery(QUrl(magnetUrl)).queryItemValue(QStringLiteral("xt"));
    const QString prefix = QStringLiteral("urn:btih:");
    if (!xt.startsWith(prefix, Qt::CaseInsensitive)) return {};
    const QString hash = xt.mid(prefix.size());
    if (hash.size() != 40) return {};
    for (const QChar character : hash) {
        if (!character.isDigit()
            && (character.toLower() < QLatin1Char('a')
                || character.toLower() > QLatin1Char('f'))) return {};
    }
    return hash.toLower();
}
}

QBitTorrentManager::QBitTorrentManager(QString dataDir, QString downloadDir, QObject *parent)
    : QObject(parent)
    , m_dataDir(std::move(dataDir))
    , m_downloadDir(std::move(downloadDir))
    , m_profileDir(QDir(m_dataDir).filePath(QStringLiteral("profile")))
{
    m_pollTimer.setInterval(500);
    connect(&m_pollTimer, &QTimer::timeout, this, &QBitTorrentManager::poll);
    connect(&m_process, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
        m_daemonReady = false;
        if (!m_stopping && m_networkReady) {
            fail(tr("The qBittorrent download service stopped unexpectedly"));
        }
    });
}

QBitTorrentManager::~QBitTorrentManager() { shutdown(); }

TorrentFileModel *QBitTorrentManager::videoFiles() { return &m_videoFiles; }
bool QBitTorrentManager::active() const { return m_active; }
bool QBitTorrentManager::backendReady() const { return m_daemonReady; }
bool QBitTorrentManager::needsFileSelection() const { return m_needsFileSelection; }
QString QBitTorrentManager::title() const { return m_title; }
QString QBitTorrentManager::selectedFileName() const { return m_selectedFileName; }
QString QBitTorrentManager::selectedFilePath() const
{
    return m_selectedFileName.isEmpty() ? QString()
                                        : QDir(m_downloadDir).filePath(m_selectedFileName);
}
qint64 QBitTorrentManager::selectedFileSize() const { return m_selectedFileSize; }
QString QBitTorrentManager::stateLabel() const { return m_stateLabel; }
QString QBitTorrentManager::errorMessage() const { return m_error; }
double QBitTorrentManager::progress() const { return m_progress; }
qint64 QBitTorrentManager::downloadRate() const { return m_downloadRate; }
int QBitTorrentManager::peerCount() const { return m_peerCount; }
int QBitTorrentManager::seedCount() const { return m_seedCount; }
double QBitTorrentManager::distributedCopies() const { return m_distributedCopies; }
double QBitTorrentManager::bufferSeconds() const { return m_bufferSeconds; }
double QBitTorrentManager::estimatedWaitSeconds() const { return m_estimatedWaitSeconds; }
bool QBitTorrentManager::bufferReady() const { return m_bufferReady; }

void QBitTorrentManager::setNetworkReady(bool ready)
{
    if (m_networkReady == ready) return;
    m_networkReady = ready;
    if (ready) {
        startDaemon();
    } else {
        const bool hadTransfer = m_active;
        stopDaemon(true);
        clearTransferState();
        if (hadTransfer) {
            m_error = tr("Download stopped because VPN protection was lost");
            m_stateLabel = tr("VPN required");
            emit stateChanged();
        }
    }
}

void QBitTorrentManager::startDaemon()
{
    if (!m_networkReady || m_process.state() != QProcess::NotRunning) return;
    if (!QFileInfo::exists(QStringLiteral("/usr/bin/qbittorrent-nox"))) {
        fail(tr("qBittorrent-nox is not installed"));
        return;
    }
    if (!QDir().mkpath(m_downloadDir) || !QDir().mkpath(m_profileDir)) {
        fail(tr("Could not create qBittorrent data directories"));
        return;
    }

    QTcpServer portProbe;
    if (!portProbe.listen(QHostAddress::LocalHost, 0)) {
        fail(tr("Could not reserve a local qBittorrent API port"));
        return;
    }
    const quint16 apiPort = portProbe.serverPort();
    portProbe.close();
    m_apiBase = QUrl(QStringLiteral("http://localhost:%1/api/v2/").arg(apiPort));
    if (!writeConfiguration()) return;

    const quint16 torrentPort = static_cast<quint16>(
        QRandomGenerator::global()->bounded(20'000, 60'000));
    m_stopping = false;
    m_daemonReady = false;
    m_process.setProgram(QStringLiteral("/usr/bin/qbittorrent-nox"));
    m_process.setArguments({QStringLiteral("--confirm-legal-notice"),
                            QStringLiteral("--profile=%1").arg(m_profileDir),
                            QStringLiteral("--webui-port=%1").arg(apiPort),
                            QStringLiteral("--torrenting-port=%1").arg(torrentPort)});
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    m_process.start();
    m_pollTimer.start();
    if (!m_active) m_stateLabel = tr("Starting qBittorrent…");
    emit stateChanged();
}

bool QBitTorrentManager::writeConfiguration()
{
    const QString directory = QDir(m_profileDir).filePath(
        QStringLiteral("qBittorrent/config"));
    if (!QDir().mkpath(directory)) {
        fail(tr("Could not create the qBittorrent configuration directory"));
        return false;
    }
    QSettings config(QDir(directory).filePath(QStringLiteral("qBittorrent.conf")),
                     QSettings::IniFormat);
    config.beginGroup(QStringLiteral("LegalNotice"));
    config.setValue(QStringLiteral("Accepted"), true);
    config.endGroup();
    config.beginGroup(QStringLiteral("Preferences"));
    config.setValue(QStringLiteral("WebUI/Enabled"), true);
    config.setValue(QStringLiteral("WebUI/Address"), QStringLiteral("127.0.0.1"));
    config.setValue(QStringLiteral("WebUI/LocalHostAuth"), false);
    config.setValue(QStringLiteral("WebUI/ServerDomains"), QStringLiteral("localhost"));
    config.setValue(QStringLiteral("WebUI/HostHeaderValidation"), true);
    config.setValue(QStringLiteral("WebUI/CSRFProtection"), true);
    config.setValue(QStringLiteral("Downloads/SavePath"), m_downloadDir + QLatin1Char('/'));
    config.setValue(QStringLiteral("Downloads/TempPath"), m_downloadDir + QLatin1Char('/'));
    config.setValue(QStringLiteral("Downloads/TempPathEnabled"), false);
    config.setValue(QStringLiteral("Connection/UPnP"), false);
    config.endGroup();
    config.beginGroup(QStringLiteral("BitTorrent"));
    config.setValue(QStringLiteral("Session/LSDEnabled"), false);
    config.setValue(QStringLiteral("Session/PortForwardingEnabled"), false);
    config.setValue(QStringLiteral("Session/QueueingSystemEnabled"), false);
    config.endGroup();
    config.sync();
    if (config.status() != QSettings::NoError) {
        fail(tr("Could not update the qBittorrent configuration"));
        return false;
    }
    return true;
}

void QBitTorrentManager::stopDaemon(bool force)
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

void QBitTorrentManager::poll()
{
    if (!m_networkReady || m_reply) return;
    if (!m_daemonReady) {
        probeApi();
    } else if (m_active) {
        requestTorrentInfo();
    }
}

void QBitTorrentManager::probeApi()
{
    get(QStringLiteral("app/version"), {}, [this](QNetworkReply *reply) {
        if (!successful(reply)) return;
        m_daemonReady = true;
        emit stateChanged();
        post(QStringLiteral("torrents/stop"),
             QUrlQuery{{QStringLiteral("hashes"), QStringLiteral("all")}},
             [this](QNetworkReply *) { submitPendingRelease(); });
    });
}

void QBitTorrentManager::startMagnet(const QString &title, const QString &magnetUrl)
{
    if (!m_networkReady) {
        fail(tr("VPN protection must be ready before starting a torrent"));
        return;
    }
    if (!magnetUrl.startsWith(QStringLiteral("magnet:?"), Qt::CaseInsensitive)) {
        fail(tr("This release does not contain a valid magnet link"));
        return;
    }
    clearTransferState();
    m_title = title;
    m_tag = QStringLiteral("dostflix-%1").arg(
        QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_hash = magnetInfoHash(magnetUrl);
    m_pendingMagnet = magnetUrl;
    m_active = true;
    m_stateLabel = tr("Submitting torrent to qBittorrent…");
    m_error.clear();
    emit stateChanged();
    startDaemon();
    if (m_daemonReady) submitPendingRelease();
}

void QBitTorrentManager::startTorrentData(const QString &title, const QByteArray &torrentData)
{
    if (!m_networkReady) {
        fail(tr("VPN protection must be ready before starting a torrent"));
        return;
    }
    if (torrentData.isEmpty()) {
        fail(tr("The torrent file is empty"));
        return;
    }
    clearTransferState();
    m_title = title;
    m_tag = QStringLiteral("dostflix-%1").arg(
        QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_hash = torrentInfoHash(torrentData);
    m_pendingTorrent = torrentData;
    m_active = true;
    m_stateLabel = tr("Submitting torrent to qBittorrent…");
    m_error.clear();
    emit stateChanged();
    startDaemon();
    if (m_daemonReady) submitPendingRelease();
}

void QBitTorrentManager::submitPendingRelease()
{
    if (!m_active || !m_daemonReady || m_reply) return;
    if (!m_hash.isEmpty() && !m_existingChecked) {
        m_existingChecked = true;
        QUrlQuery query{{QStringLiteral("hashes"), m_hash}};
        get(QStringLiteral("torrents/info"), query, [this](QNetworkReply *reply) {
            if (!successful(reply)) {
                fail(reply->errorString());
                return;
            }
            const QJsonArray existing = QJsonDocument::fromJson(reply->readAll()).array();
            if (existing.isEmpty()) {
                submitPendingRelease();
                return;
            }
            m_pendingMagnet.clear();
            m_pendingTorrent.clear();
            m_stateLabel = tr("Resuming existing qBittorrent download…");
            emit stateChanged();
            requestTorrentInfo();
        });
        return;
    }
    if (!m_pendingMagnet.isEmpty()) {
        QUrlQuery form;
        form.addQueryItem(QStringLiteral("urls"), m_pendingMagnet);
        form.addQueryItem(QStringLiteral("savepath"), m_downloadDir);
        form.addQueryItem(QStringLiteral("tags"), m_tag);
        // Magnet metadata itself comes from peers, so magnets must initially
        // run. File priorities are narrowed immediately after metadata arrives.
        form.addQueryItem(QStringLiteral("stopped"), QStringLiteral("false"));
        form.addQueryItem(QStringLiteral("sequentialDownload"), QStringLiteral("true"));
        form.addQueryItem(QStringLiteral("firstLastPiecePrio"), QStringLiteral("true"));
        post(QStringLiteral("torrents/add"), form, [this](QNetworkReply *reply) {
            if (!successful(reply)) {
                fail(reply->errorString());
                return;
            }
            m_pendingMagnet.clear();
            m_stateLabel = tr("Acquiring torrent metadata…");
            emit stateChanged();
        });
        return;
    }

    auto *multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart torrent;
    torrent.setHeader(QNetworkRequest::ContentDispositionHeader,
                      QStringLiteral("form-data; name=\"torrents\"; filename=\"release.torrent\""));
    torrent.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-bittorrent"));
    torrent.setBody(m_pendingTorrent);
    multi->append(torrent);
    multi->append(textPart("savepath", m_downloadDir.toUtf8()));
    multi->append(textPart("tags", m_tag.toUtf8()));
    multi->append(textPart("stopped", "true"));
    multi->append(textPart("sequentialDownload", "true"));
    multi->append(textPart("firstLastPiecePrio", "true"));
    QNetworkReply *reply = m_network.post(apiRequest(QStringLiteral("torrents/add")), multi);
    multi->setParent(reply);
    m_reply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        if (!successful(reply)) {
            fail(reply->errorString());
        } else {
            m_pendingTorrent.clear();
            m_stateLabel = tr("Loading torrent metadata…");
            emit stateChanged();
        }
        reply->deleteLater();
    });
}

void QBitTorrentManager::requestTorrentInfo()
{
    QUrlQuery query;
    if (!m_hash.isEmpty()) query.addQueryItem(QStringLiteral("hashes"), m_hash);
    else query.addQueryItem(QStringLiteral("tag"), m_tag);
    get(QStringLiteral("torrents/info"), query, [this](QNetworkReply *reply) {
        if (!successful(reply)) return;
        const QJsonArray torrents = QJsonDocument::fromJson(reply->readAll()).array();
        if (torrents.isEmpty()) return;
        const QJsonObject torrent = torrents.first().toObject();
        m_hash = torrent.value(QStringLiteral("hash")).toString();
        m_progress = torrent.value(QStringLiteral("progress")).toDouble();
        m_downloadRate = static_cast<qint64>(torrent.value(QStringLiteral("dlspeed")).toDouble());
        m_peerCount = torrent.value(QStringLiteral("num_leechs")).toInt();
        m_seedCount = torrent.value(QStringLiteral("num_seeds")).toInt();
        const double availability = torrent.value(QStringLiteral("availability")).toDouble(-1.0);
        m_distributedCopies = std::isfinite(availability) && availability >= 0.0
            ? availability : 0.0;
        const QString state = torrent.value(QStringLiteral("state")).toString();
        if (state == QStringLiteral("error") || state == QStringLiteral("missingFiles")) {
            fail(tr("qBittorrent reported a download error"));
            return;
        }
        emit statisticsChanged();
        if (m_pieceSize == 0) requestProperties();
        else if (m_videoFiles.rowCount() == 0) requestFiles();
        else if (m_selectedTorrentIndex >= 0) requestPieceStates();
    });
}

void QBitTorrentManager::requestProperties()
{
    QUrlQuery query{{QStringLiteral("hash"), m_hash}};
    get(QStringLiteral("torrents/properties"), query, [this](QNetworkReply *reply) {
        if (!successful(reply)) return;
        const QJsonObject properties = QJsonDocument::fromJson(reply->readAll()).object();
        m_pieceSize = static_cast<qint64>(
            properties.value(QStringLiteral("piece_size")).toDouble());
        if (m_pieceSize <= 0) {
            // Magnet entries exist before their metadata has arrived. During
            // that normal state qBittorrent reports a zero piece size; keep
            // polling instead of turning metadata acquisition into an error.
            m_pieceSize = 0;
            m_stateLabel = tr("Acquiring torrent metadata…");
            emit stateChanged();
            return;
        }
        requestFiles();
    });
}

void QBitTorrentManager::requestFiles()
{
    QUrlQuery query{{QStringLiteral("hash"), m_hash}};
    get(QStringLiteral("torrents/files"), query, [this](QNetworkReply *reply) {
        if (!successful(reply)) return;
        const QJsonArray files = QJsonDocument::fromJson(reply->readAll()).array();
        if (files.isEmpty()) return;
        QList<ApiFile> apiFiles;
        for (const QJsonValue &value : files) {
            const QJsonObject item = value.toObject();
            const int index = item.value(QStringLiteral("index")).toInt(-1);
            const QString path = item.value(QStringLiteral("name")).toString();
            const qint64 size = static_cast<qint64>(
                item.value(QStringLiteral("size")).toDouble());
            if (index < 0 || path.isEmpty() || size < 0) continue;
            apiFiles.push_back({index, path, size, 0});
        }
        std::sort(apiFiles.begin(), apiFiles.end(), [](const ApiFile &left,
                                                       const ApiFile &right) {
            return left.index < right.index;
        });
        m_apiFiles.clear();
        m_fileOffsets.clear();
        std::vector<TorrentVideoFile> videos;
        qint64 offset = 0;
        for (ApiFile &file : apiFiles) {
            file.offset = offset;
            m_apiFiles.push_back(file);
            m_fileOffsets.insert(file.index, offset);
            if (isVideoFile(file.path, file.size)) {
                videos.push_back({file.index, file.path, file.size});
            }
            offset += file.size;
        }
        m_videoFiles.replace(videos);
        if (videos.empty()) {
            fail(tr("No playable video file was found in this torrent"));
        } else if (videos.size() == 1) {
            applyFileSelection(videos.front());
        } else {
            m_needsFileSelection = true;
            m_stateLabel = tr("Choose a video file");
            emit stateChanged();
        }
    });
}

void QBitTorrentManager::selectVideoFile(int row)
{
    const TorrentVideoFile *file = m_videoFiles.at(row);
    if (file && m_active && !m_hash.isEmpty()) applyFileSelection(*file);
}

void QBitTorrentManager::applyFileSelection(const TorrentVideoFile &file)
{
    m_selectedTorrentIndex = file.torrentIndex;
    m_selectedFileName = file.path;
    m_selectedFileSize = file.sizeBytes;
    m_selectedFileOffset = m_fileOffsets.value(file.torrentIndex, 0);
    m_needsFileSelection = false;
    m_stateLabel = tr("Applying qBittorrent file priorities…");
    emit stateChanged();

    QStringList unwanted;
    for (const ApiFile &candidate : std::as_const(m_apiFiles)) {
        if (candidate.index != file.torrentIndex) unwanted.push_back(QString::number(candidate.index));
    }
    const auto prioritizeSelected = [this] {
        QUrlQuery selected;
        selected.addQueryItem(QStringLiteral("hash"), m_hash);
        selected.addQueryItem(QStringLiteral("id"), QString::number(m_selectedTorrentIndex));
        selected.addQueryItem(QStringLiteral("priority"), QStringLiteral("7"));
        post(QStringLiteral("torrents/filePrio"), selected,
             [this](QNetworkReply *reply) {
            if (!successful(reply)) fail(reply->errorString());
            else startSelectedFile();
        });
    };
    if (unwanted.isEmpty()) {
        prioritizeSelected();
        return;
    }
    QUrlQuery excluded;
    excluded.addQueryItem(QStringLiteral("hash"), m_hash);
    excluded.addQueryItem(QStringLiteral("id"), unwanted.join(QLatin1Char('|')));
    excluded.addQueryItem(QStringLiteral("priority"), QStringLiteral("0"));
    post(QStringLiteral("torrents/filePrio"), excluded,
         [this, prioritizeSelected](QNetworkReply *reply) {
        if (!successful(reply)) fail(reply->errorString());
        else prioritizeSelected();
    });
}

void QBitTorrentManager::startSelectedFile()
{
    QUrlQuery form{{QStringLiteral("hashes"), m_hash}};
    post(QStringLiteral("torrents/start"), form, [this](QNetworkReply *reply) {
        if (!successful(reply)) {
            fail(reply->errorString());
            return;
        }
        m_stateLabel = tr("Building a safe playback buffer…");
        emit stateChanged();
    });
}

void QBitTorrentManager::requestPieceStates()
{
    QUrlQuery query{{QStringLiteral("hash"), m_hash}};
    get(QStringLiteral("torrents/pieceStates"), query, [this](QNetworkReply *reply) {
        if (!successful(reply)) return;
        m_pieceStates.clear();
        for (const QJsonValue &state : QJsonDocument::fromJson(reply->readAll()).array()) {
            m_pieceStates.push_back(state.toInt());
        }
        updateBuffer();
    });
}

void QBitTorrentManager::updateBuffer()
{
    if (m_pieceSize <= 0 || m_pieceStates.isEmpty() || m_selectedFileSize <= 0) return;
    qint64 contiguous = 0;
    qint64 absolute = m_selectedFileOffset;
    while (contiguous < m_selectedFileSize) {
        const qint64 piece = absolute / m_pieceSize;
        if (piece < 0 || piece >= m_pieceStates.size() || m_pieceStates.at(piece) != 2) break;
        const qint64 withinPiece = absolute % m_pieceSize;
        const qint64 bytes = std::min(m_pieceSize - withinPiece,
                                      m_selectedFileSize - contiguous);
        contiguous += bytes;
        absolute += bytes;
    }
    const BufferEstimate estimate = BufferController::estimate(
        contiguous, m_selectedFileSize, 8'000'000, m_downloadRate);
    m_bufferSeconds = estimate.playableSeconds;
    m_estimatedWaitSeconds = estimate.estimatedWaitSeconds;
    m_bufferReady = estimate.ready;
    if (m_bufferReady) m_stateLabel = tr("Ready to play");
    emit statisticsChanged();
    emit stateChanged();
}

bool QBitTorrentManager::isRangeAvailable(qint64 offset, qint64 length) const
{
    if (m_pieceSize <= 0 || offset < 0 || length <= 0
        || offset + length > m_selectedFileSize || m_pieceStates.isEmpty()) return false;
    const qint64 first = (m_selectedFileOffset + offset) / m_pieceSize;
    const qint64 last = (m_selectedFileOffset + offset + length - 1) / m_pieceSize;
    for (qint64 piece = first; piece <= last; ++piece) {
        if (piece < 0 || piece >= m_pieceStates.size() || m_pieceStates.at(piece) != 2) return false;
    }
    return true;
}

void QBitTorrentManager::prioritizeRange(qint64 offset, qint64 length)
{
    Q_UNUSED(offset)
    Q_UNUSED(length)
    // qBittorrent owns piece picking. Sequential mode plus first/last priority
    // is enabled when the torrent is added; arbitrary seek reprioritization is
    // intentionally deferred until its API exposes a stable piece-priority call.
}

void QBitTorrentManager::cancel()
{
    if (m_daemonReady && !m_hash.isEmpty() && !m_reply) {
        QUrlQuery form;
        form.addQueryItem(QStringLiteral("hashes"), m_hash);
        form.addQueryItem(QStringLiteral("deleteFiles"), QStringLiteral("false"));
        post(QStringLiteral("torrents/delete"), form, [](QNetworkReply *) {});
    }
    clearTransferState();
}

void QBitTorrentManager::shutdown()
{
    m_networkReady = false;
    stopDaemon(false);
    clearTransferState();
}

void QBitTorrentManager::clearTransferState()
{
    m_videoFiles.replace({});
    m_apiFiles.clear();
    m_fileOffsets.clear();
    m_pieceStates.clear();
    m_hash.clear();
    m_tag.clear();
    m_pendingMagnet.clear();
    m_pendingTorrent.clear();
    m_existingChecked = false;
    m_selectedFileName.clear();
    m_stateLabel.clear();
    m_error.clear();
    m_active = false;
    m_needsFileSelection = false;
    m_bufferReady = false;
    m_selectedTorrentIndex = -1;
    m_selectedFileSize = 0;
    m_selectedFileOffset = 0;
    m_pieceSize = 0;
    m_progress = 0.0;
    m_downloadRate = 0;
    m_peerCount = 0;
    m_seedCount = 0;
    m_distributedCopies = 0.0;
    m_bufferSeconds = 0.0;
    m_estimatedWaitSeconds = 0.0;
    emit stateChanged();
    emit statisticsChanged();
}

void QBitTorrentManager::fail(QString error)
{
    m_error = std::move(error);
    m_stateLabel = tr("Download error");
    m_active = false;
    emit stateChanged();
}

QNetworkRequest QBitTorrentManager::apiRequest(const QString &path) const
{
    QUrl url = m_apiBase.resolved(QUrl(path));
    QNetworkRequest request(url);
    request.setRawHeader("Referer", m_apiBase.toString().toUtf8());
    request.setTransferTimeout(10'000);
    return request;
}

void QBitTorrentManager::get(const QString &path, const QUrlQuery &query,
                             std::function<void(QNetworkReply *)> finished)
{
    QUrl url = m_apiBase.resolved(QUrl(path));
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader("Referer", m_apiBase.toString().toUtf8());
    request.setTransferTimeout(10'000);
    QNetworkReply *reply = m_network.get(request);
    m_reply = reply;
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, finished = std::move(finished)] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        finished(reply);
        reply->deleteLater();
    });
}

void QBitTorrentManager::post(const QString &path, const QUrlQuery &form,
                              std::function<void(QNetworkReply *)> finished)
{
    QNetworkRequest request = apiRequest(path);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));
    QNetworkReply *reply = m_network.post(request, form.query(QUrl::FullyEncoded).toUtf8());
    m_reply = reply;
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, finished = std::move(finished)] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        finished(reply);
        reply->deleteLater();
    });
}
