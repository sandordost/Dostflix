#include "streaming/TorrentEngine.h"

#include "streaming/BufferController.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/download_priority.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/load_torrent.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_info.hpp>
#include <limits>

namespace lt = libtorrent;

struct TorrentEngine::Impl final
{
    std::unique_ptr<lt::session> session;
    lt::torrent_handle handle;
};

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

lt::settings_pack torrentSettings()
{
    lt::settings_pack settings;
    settings.set_int(lt::settings_pack::alert_mask,
                     lt::alert_category::error | lt::alert_category::status
                         | lt::alert_category::storage);
    settings.set_bool(lt::settings_pack::enable_upnp, false);
    settings.set_bool(lt::settings_pack::enable_natpmp, false);
    settings.set_bool(lt::settings_pack::enable_lsd, false);
    settings.set_bool(lt::settings_pack::enable_dht, true);
    settings.set_str(lt::settings_pack::user_agent, "Dostflix/0.1");
    return settings;
}

void prepareForManualDownload(lt::add_torrent_params &params)
{
    params.flags &= ~lt::torrent_flags::paused;
    params.flags &= ~lt::torrent_flags::auto_managed;
    // Metadata is still acquired, but payload cannot start before the user has
    // selected the video and the disk thread has applied its file priorities.
    params.flags |= lt::torrent_flags::upload_mode;
}
}

TorrentEngine::TorrentEngine(QString downloadDir, QObject *parent)
    : QObject(parent), m_downloadDir(std::move(downloadDir)), m_impl(std::make_unique<Impl>())
{
    m_pollTimer.setInterval(500);
    connect(&m_pollTimer, &QTimer::timeout, this, &TorrentEngine::poll);
}

TorrentEngine::~TorrentEngine() { shutdown(); }

TorrentFileModel *TorrentEngine::videoFiles() { return &m_videoFiles; }
bool TorrentEngine::active() const { return m_active; }
bool TorrentEngine::needsFileSelection() const { return m_needsFileSelection; }
QString TorrentEngine::title() const { return m_title; }
QString TorrentEngine::selectedFileName() const { return m_selectedFileName; }
QString TorrentEngine::selectedFilePath() const
{
    return m_selectedFileName.isEmpty() ? QString()
        : QDir(m_downloadDir).filePath(m_selectedFileName);
}
qint64 TorrentEngine::selectedFileSize() const { return m_selectedFileSize; }
QString TorrentEngine::stateLabel() const { return m_stateLabel; }
QString TorrentEngine::errorMessage() const { return m_error; }
double TorrentEngine::progress() const { return m_progress; }
qint64 TorrentEngine::downloadRate() const { return m_downloadRate; }
int TorrentEngine::peerCount() const { return m_peerCount; }
int TorrentEngine::seedCount() const { return m_seedCount; }
double TorrentEngine::distributedCopies() const { return m_distributedCopies; }
double TorrentEngine::bufferSeconds() const { return m_bufferSeconds; }
double TorrentEngine::estimatedWaitSeconds() const { return m_estimatedWaitSeconds; }
bool TorrentEngine::bufferReady() const { return m_bufferReady; }

void TorrentEngine::setNetworkReady(bool ready)
{
    if (m_networkReady == ready) return;
    m_networkReady = ready;
    if (!ready && m_active) {
        cancel();
        m_error = tr("Torrent stopped because VPN protection was lost");
        m_stateLabel = tr("VPN required");
        emit stateChanged();
    }
}

bool TorrentEngine::isRangeAvailable(qint64 offset, qint64 length) const
{
    if (!m_impl->handle.is_valid() || m_selectedTorrentIndex < 0
        || offset < 0 || length <= 0 || offset + length > m_selectedFileSize) {
        return false;
    }
    const std::shared_ptr<const lt::torrent_info> info = m_impl->handle.torrent_file();
    if (!info) return false;
    const lt::file_storage &storage = info->layout();
    const lt::file_index_t fileIndex{m_selectedTorrentIndex};
    const lt::peer_request first = storage.map_file(fileIndex, offset, 1);
    const lt::peer_request last = storage.map_file(fileIndex, offset + length - 1, 1);
    for (lt::piece_index_t piece = first.piece; piece <= last.piece; ++piece) {
        if (!m_impl->handle.have_piece(piece)) return false;
    }
    return true;
}

void TorrentEngine::prioritizeRange(qint64 offset, qint64 length)
{
    if (!m_impl->handle.is_valid() || m_selectedTorrentIndex < 0
        || offset < 0 || length <= 0 || offset >= m_selectedFileSize) return;
    const std::shared_ptr<const lt::torrent_info> info = m_impl->handle.torrent_file();
    if (!info) return;
    const lt::file_storage &storage = info->layout();
    const lt::file_index_t fileIndex{m_selectedTorrentIndex};
    const lt::peer_request first = storage.map_file(fileIndex, offset, 1);
    const lt::peer_request last = storage.map_file(
        fileIndex, std::min(m_selectedFileSize - 1, offset + length - 1), 1);
    int deadline = 0;
    for (lt::piece_index_t piece = first.piece; piece <= last.piece; ++piece) {
        m_impl->handle.set_piece_deadline(piece, deadline);
        deadline += 25;
    }
}

void TorrentEngine::startMagnet(const QString &title, const QString &magnetUrl)
{
    if (!m_networkReady) {
        fail(tr("VPN protection must be ready before starting a torrent"));
        return;
    }
    if (!magnetUrl.startsWith(QStringLiteral("magnet:?"), Qt::CaseInsensitive)) {
        fail(tr("This release does not provide a magnet link yet"));
        return;
    }
    cancel();
    if (!QDir().mkpath(m_downloadDir)) {
        fail(tr("Could not create the download directory"));
        return;
    }

    lt::error_code error;
    lt::add_torrent_params params = lt::parse_magnet_uri(magnetUrl.toStdString(), error);
    if (error) {
        fail(QString::fromStdString(error.message()));
        return;
    }
    params.save_path = m_downloadDir.toStdString();
    prepareForManualDownload(params);
    m_impl->session = std::make_unique<lt::session>(torrentSettings());
    m_impl->session->async_add_torrent(std::move(params));
    m_title = title;
    m_stateLabel = tr("Acquiring torrent metadata…");
    m_error.clear();
    m_active = true;
    m_pollTimer.start();
    emit stateChanged();
}

void TorrentEngine::selectVideoFile(int row)
{
    const TorrentVideoFile *file = m_videoFiles.at(row);
    if (file == nullptr || !m_active || !m_impl->handle.is_valid()) return;
    applyFileSelection(*file);
}

void TorrentEngine::startTorrentData(const QString &title, const QByteArray &torrentData)
{
    if (!m_networkReady) {
        fail(tr("VPN protection must be ready before starting a torrent"));
        return;
    }
    cancel();
    if (!QDir().mkpath(m_downloadDir)) {
        fail(tr("Could not create the download directory"));
        return;
    }
    lt::error_code error;
    const lt::span<const char> buffer(torrentData.constData(), torrentData.size());
    lt::add_torrent_params params = lt::load_torrent_buffer(
        buffer, error, lt::load_torrent_limits{});
    if (error) {
        fail(QString::fromStdString(error.message()));
        return;
    }
    params.save_path = m_downloadDir.toStdString();
    prepareForManualDownload(params);
    m_impl->session = std::make_unique<lt::session>(torrentSettings());
    m_impl->session->async_add_torrent(std::move(params));
    m_title = title;
    m_stateLabel = tr("Loading torrent…");
    m_error.clear();
    m_active = true;
    m_pollTimer.start();
    emit stateChanged();
}

void TorrentEngine::cancel()
{
    m_pollTimer.stop();
    if (m_impl->session) m_impl->session->pause();
    m_impl->handle = {};
    m_impl->session.reset();
    m_videoFiles.replace({});
    m_active = false;
    m_needsFileSelection = false;
    m_selectedTorrentIndex = -1;
    m_selectedFileSize = 0;
    m_selectedFileName.clear();
    m_progress = 0.0;
    m_downloadRate = 0;
    m_peerCount = 0;
    m_seedCount = 0;
    m_distributedCopies = 0.0;
    m_bufferSeconds = 0.0;
    m_estimatedWaitSeconds = 0.0;
    m_bufferReady = false;
    m_waitingFilePriorities = false;
    m_stateLabel.clear();
    m_error.clear();
    emit stateChanged();
    emit statisticsChanged();
}

void TorrentEngine::shutdown() { cancel(); }

void TorrentEngine::poll()
{
    if (!m_impl->session) return;
    std::vector<lt::alert *> alerts;
    m_impl->session->pop_alerts(&alerts);
    for (lt::alert *alert : alerts) {
        if (const auto *added = lt::alert_cast<lt::add_torrent_alert>(alert)) {
            if (added->error) {
                fail(QString::fromStdString(added->error.message()));
                return;
            }
            m_impl->handle = added->handle;
            if (m_impl->handle.status().has_metadata) collectVideoFiles();
        } else if (lt::alert_cast<lt::metadata_received_alert>(alert)) {
            collectVideoFiles();
        } else if (const auto *filePriorities = lt::alert_cast<lt::file_prio_alert>(alert)) {
            if (filePriorities->handle == m_impl->handle && m_waitingFilePriorities) {
                if (filePriorities->error) {
                    fail(QString::fromStdString(filePriorities->error.message()));
                    return;
                }
                finalizeFileSelection();
            }
        } else if (const auto *torrentError = lt::alert_cast<lt::torrent_error_alert>(alert)) {
            fail(QString::fromStdString(torrentError->error.message()));
            return;
        } else if (const auto *fileError = lt::alert_cast<lt::file_error_alert>(alert)) {
            fail(QString::fromStdString(fileError->error.message()));
            return;
        }
    }
    updateStatistics();
}

void TorrentEngine::collectVideoFiles()
{
    const std::shared_ptr<const lt::torrent_info> info = m_impl->handle.torrent_file();
    if (!info) return;
    const lt::file_storage &storage = info->layout();
    std::vector<TorrentVideoFile> videos;
    for (lt::file_index_t index : storage.file_range()) {
        const qint64 size = storage.file_size(index);
        const QString path = QString::fromStdString(storage.file_path(index));
        if (isVideoFile(path, size)) videos.push_back({static_cast<int>(index), path, size});
    }
    m_videoFiles.replace(videos);
    if (videos.empty()) {
        fail(tr("No playable video file was found in this torrent"));
        return;
    }
    if (videos.size() == 1) {
        applyFileSelection(videos.front());
    } else {
        m_needsFileSelection = true;
        m_stateLabel = tr("Choose a video file");
        emit stateChanged();
    }
}

void TorrentEngine::applyFileSelection(const TorrentVideoFile &file)
{
    const std::shared_ptr<const lt::torrent_info> info = m_impl->handle.torrent_file();
    if (!info) return;
    const lt::file_storage &storage = info->layout();
    std::vector<lt::download_priority_t> priorities(
        static_cast<std::size_t>(storage.num_files()), lt::dont_download);
    priorities.at(static_cast<std::size_t>(file.torrentIndex)) = lt::default_priority;
    m_impl->handle.prioritize_files(priorities);

    m_selectedTorrentIndex = file.torrentIndex;
    m_selectedFileSize = file.sizeBytes;
    m_selectedFileName = file.path;
    m_needsFileSelection = false;
    m_waitingFilePriorities = true;
    m_stateLabel = tr("Preparing the selected video…");
    emit stateChanged();
}

void TorrentEngine::finalizeFileSelection()
{
    const std::shared_ptr<const lt::torrent_info> info = m_impl->handle.torrent_file();
    if (!info || m_selectedTorrentIndex < 0) return;
    const lt::file_storage &storage = info->layout();
    const lt::file_index_t fileIndex{m_selectedTorrentIndex};
    std::vector<std::pair<lt::piece_index_t, lt::download_priority_t>> priorities;
    const auto prioritizeRange = [&](qint64 offset, qint64 length) {
        if (length <= 0) return;
        const lt::peer_request first = storage.map_file(fileIndex, offset, 1);
        const lt::peer_request last = storage.map_file(
            fileIndex, std::min(m_selectedFileSize - 1, offset + length - 1), 1);
        for (lt::piece_index_t piece = first.piece; piece <= last.piece; ++piece) {
            priorities.emplace_back(piece, lt::top_priority);
        }
    };
    prioritizeRange(0, std::min<qint64>(m_selectedFileSize, 16LL * 1024 * 1024));
    prioritizeRange(std::max<qint64>(0, m_selectedFileSize - 4LL * 1024 * 1024),
                    std::min<qint64>(m_selectedFileSize, 4LL * 1024 * 1024));
    m_impl->handle.prioritize_pieces(priorities);

    m_impl->handle.unset_flags(lt::torrent_flags::upload_mode
                               | lt::torrent_flags::share_mode
                               | lt::torrent_flags::auto_managed
                               | lt::torrent_flags::paused);
    m_impl->handle.resume();
    m_impl->handle.force_reannounce(0, lt::torrent_handle::high_priority);
    m_waitingFilePriorities = false;
    m_stateLabel = tr("Building a safe playback buffer…");
    emit stateChanged();
}

void TorrentEngine::updateStatistics()
{
    if (!m_impl->handle.is_valid()) return;
    const lt::torrent_status status = m_impl->handle.status();
    m_downloadRate = status.download_payload_rate;
    m_peerCount = status.num_peers;
    m_seedCount = status.num_seeds;
    m_distributedCopies = status.distributed_copies;
    m_progress = status.total_wanted > 0
        ? static_cast<double>(status.total_wanted_done)
              / static_cast<double>(status.total_wanted)
        : 0.0;

    if (m_selectedTorrentIndex >= 0 && status.has_metadata) {
        const std::shared_ptr<const lt::torrent_info> info = m_impl->handle.torrent_file();
        const lt::file_storage &storage = info->layout();
        const lt::file_index_t fileIndex{m_selectedTorrentIndex};
        const lt::peer_request first = storage.map_file(fileIndex, 0, 1);
        const lt::peer_request last = storage.map_file(fileIndex, m_selectedFileSize - 1, 1);
        qint64 contiguous = 0;
        for (lt::piece_index_t piece = first.piece; piece <= last.piece; ++piece) {
            if (!m_impl->handle.have_piece(piece)) break;
            qint64 available = storage.piece_size(piece);
            if (piece == first.piece) available -= first.start;
            contiguous += std::min(available, m_selectedFileSize - contiguous);
            if (contiguous >= m_selectedFileSize) break;
        }
        contiguous = std::min(contiguous, m_selectedFileSize);
        // Until media probing is connected, use a conservative 8 Mbit/s estimate.
        const BufferEstimate estimate = BufferController::estimate(
            contiguous, m_selectedFileSize, 8'000'000, m_downloadRate);
        m_bufferSeconds = estimate.playableSeconds;
        m_estimatedWaitSeconds = estimate.estimatedWaitSeconds;
        m_bufferReady = estimate.ready;
        if (m_bufferReady) m_stateLabel = tr("Ready to play");
    }
    emit statisticsChanged();
    emit stateChanged();
}

void TorrentEngine::fail(QString error)
{
    cancel();
    m_error = std::move(error);
    m_stateLabel = tr("Torrent error");
    emit stateChanged();
}
