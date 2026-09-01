#include "library/DownloadManager.h"

#include "library/LibraryManager.h"

#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHostAddress>
#include <QStorageInfo>
#include <QUrlQuery>
#include <algorithm>
#include <iterator>
#include <fcntl.h>
#include <utility>
#include <unistd.h>

namespace {
constexpr qint64 DiskSafetyMargin = 512LL * 1024 * 1024;

QString magnetInfoHash(const QString &magnetUrl)
{
    const QUrl url(magnetUrl);
    for (const auto &[key, value] : QUrlQuery(url).queryItems()) {
        if (key.compare(QStringLiteral("xt"), Qt::CaseInsensitive) != 0
            || !value.startsWith(QStringLiteral("urn:btih:"), Qt::CaseInsensitive)) continue;
        const QByteArray encoded = value.mid(9).toLatin1().toUpper();
        if (encoded.size() == 40) return QString::fromLatin1(encoded).toLower();
        if (encoded.size() != 32) return {};
        static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
        QByteArray decoded;
        quint32 bits = 0;
        int bitCount = 0;
        for (const char character : encoded) {
            const char *position = std::find(std::begin(alphabet), std::end(alphabet) - 1,
                                             character);
            if (position == std::end(alphabet) - 1) return {};
            bits = (bits << 5) | static_cast<quint32>(position - alphabet);
            bitCount += 5;
            if (bitCount >= 8) {
                bitCount -= 8;
                decoded.append(static_cast<char>((bits >> bitCount) & 0xff));
            }
        }
        return decoded.size() == 20 ? QString::fromLatin1(decoded.toHex()) : QString();
    }
    return {};
}
}

DownloadManager::DownloadManager(LibraryDatabase &database, LibraryManager &library,
                                 QObject *parent)
    : QObject(parent), m_database(database), m_library(library)
{
    loadPending();
    refreshDiskSpace();
}

DownloadManager::~DownloadManager() { pause(); }
bool DownloadManager::active() const { return m_active; }
bool DownloadManager::hasPending() const
{
    return !m_transfer.torrentHash.isEmpty()
        && m_transfer.state != QStringLiteral("completed");
}
bool DownloadManager::hasTransfer() const { return !m_transfer.torrentHash.isEmpty(); }
bool DownloadManager::playable() const
{
    return hasTransfer() && (QFileInfo(m_transfer.finalPath).isFile()
                             || m_transfer.bytesWritten > 0);
}
QString DownloadManager::title() const { return m_transfer.title; }
qint64 DownloadManager::bytesWritten() const { return m_transfer.bytesWritten; }
qint64 DownloadManager::expectedSize() const { return m_transfer.expectedSize; }
qint64 DownloadManager::bytesRemaining() const { return m_bytesRemaining; }
qint64 DownloadManager::availableBytes() const { return m_availableBytes; }
bool DownloadManager::diskSpaceReady() const { return m_diskSpaceReady; }
QString DownloadManager::partialFileName() const
{ return QFileInfo(m_transfer.partialPath).fileName(); }
double DownloadManager::progress() const
{
    return m_transfer.expectedSize > 0
        ? qBound(0.0, static_cast<double>(m_transfer.bytesWritten)
                          / static_cast<double>(m_transfer.expectedSize), 1.0)
        : 0.0;
}
QString DownloadManager::stateLabel() const { return m_stateLabel; }
QString DownloadManager::errorMessage() const { return m_error; }

void DownloadManager::setNetworkReady(bool ready)
{
    if (m_networkReady == ready) return;
    m_networkReady = ready;
    if (!ready) {
        m_sourceUrl.clear();
        pause();
        if (hasPending()) m_stateLabel = tr("Paused until VPN protection returns");
        emit stateChanged();
        return;
    }
    resume();
}

void DownloadManager::beginTransfer(const QString &title, const QString &torrentHash,
                                    int fileIndex, const QString &fileName,
                                    qint64 expectedSize, const QUrl &sourceUrl)
{
    if (!m_networkReady || torrentHash.isEmpty() || fileIndex < 0
        || expectedSize <= 0 || !sourceUrl.isValid()) return;
    const QHostAddress sourceAddress(sourceUrl.host());
    if (sourceUrl.scheme() != QStringLiteral("http") || !sourceAddress.isLoopback()) {
        m_error = tr("The durable download source is not a loopback TorrServer URL");
        m_stateLabel = tr("Download blocked");
        emit stateChanged();
        return;
    }
    if (m_reply) pause();

    const std::optional<LibraryTransfer> existing = m_database.transfer(torrentHash, fileIndex);
    if (existing && existing->state == QStringLiteral("completed")
        && QFileInfo(existing->finalPath).isFile()
        && QFileInfo(existing->finalPath).size() == expectedSize) {
        m_transfer = *existing;
        m_transfer.bytesWritten = expectedSize;
        refreshDiskSpace();
        m_stateLabel = tr("Already saved to library");
        m_error.clear();
        m_library.refresh();
        emit stateChanged();
        return;
    }
    if (existing && existing->state != QStringLiteral("completed")) {
        m_transfer = *existing;
        m_transfer.expectedSize = expectedSize;
        m_transfer.fileName = fileName;
        m_transfer.title = title;
    } else {
        const QString finalPath = chooseFinalPath(fileName, torrentHash);
        m_transfer = {torrentHash, fileIndex, title, QFileInfo(fileName).fileName(),
                      expectedSize, finalPath + QStringLiteral(".dostflix.part"),
                      finalPath, 0, QStringLiteral("pending")};
    }
    m_sourceUrl = sourceUrl;
    m_error.clear();

    const QString root = QDir(m_library.directory()).absolutePath() + QLatin1Char('/');
    if (!QFileInfo(m_transfer.partialPath).absoluteFilePath().startsWith(root)
        || !QFileInfo(m_transfer.finalPath).absoluteFilePath().startsWith(root)
        || QFileInfo(m_transfer.partialPath).isSymLink()
        || QFileInfo(m_transfer.finalPath).isSymLink()) {
        fail(tr("The saved download path is outside the movie library"));
        return;
    }

    const QFileInfo finalInfo(m_transfer.finalPath);
    if (finalInfo.isFile() && finalInfo.size() == expectedSize) {
        m_transfer.bytesWritten = expectedSize;
        completeTransfer();
        return;
    }
    const QFileInfo partialInfo(m_transfer.partialPath);
    m_transfer.bytesWritten = partialInfo.isFile() ? partialInfo.size() : 0;
    if (m_transfer.bytesWritten > expectedSize) {
        fail(tr("The partial download is larger than the expected video"));
        return;
    }
    refreshDiskSpace();
    if (!persist(QStringLiteral("pending"))) {
        m_stateLabel = tr("Could not save download state");
        emit stateChanged();
        return;
    }
    startRequest();
}

void DownloadManager::pause()
{
    m_pausing = true;
    if (m_reply) {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
    if (m_file.isOpen()) {
        m_file.flush();
        if (m_file.handle() >= 0) ::fsync(m_file.handle());
        m_file.close();
    }
    m_active = false;
    if (hasPending() && m_transfer.state != QStringLiteral("completed")) {
        persist(QStringLiteral("paused"));
        if (m_networkReady) m_stateLabel = tr("Download paused");
    }
    m_pausing = false;
    emit stateChanged();
}

void DownloadManager::resume()
{
    if (m_active || !hasPending()) return;
    if (!m_networkReady) {
        m_stateLabel = tr("Paused until VPN protection returns");
        emit stateChanged();
        return;
    }
    m_error.clear();
    if (!ensureDiskSpace()) return;
    if (!m_sourceUrl.isEmpty()) {
        startRequest();
        return;
    }
    m_stateLabel = tr("Restoring saved torrent…");
    emit stateChanged();
    emit resumeRequested(m_transfer.title, m_transfer.torrentHash, m_transfer.fileIndex,
                         m_transfer.fileName, m_transfer.expectedSize);
}

void DownloadManager::play()
{
    if (!playable()) return;
    if (QFileInfo(m_transfer.finalPath).isFile()
        && QFileInfo(m_transfer.finalPath).size() == m_transfer.expectedSize) {
        emit localPlaybackRequested(QUrl::fromLocalFile(m_transfer.finalPath), m_transfer.title);
        return;
    }
    if (!m_networkReady) {
        m_error = tr("VPN protection is required to continue this partial download");
        emit stateChanged();
        return;
    }
    emit torrentPlaybackRequested(m_transfer.title, m_transfer.torrentHash,
                                  m_transfer.fileIndex, m_transfer.fileName,
                                  m_transfer.expectedSize);
}

void DownloadManager::remove()
{
    if (!hasTransfer()) return;
    pause();
    if (!pathsAreSafe()) {
        m_error = tr("The saved download path is outside the movie library");
        emit stateChanged();
        return;
    }
    for (const QString &path : {m_transfer.partialPath, m_transfer.finalPath}) {
        if (QFileInfo::exists(path) && !QFile::remove(path)) {
            m_error = tr("Could not remove %1").arg(path);
            emit stateChanged();
            return;
        }
    }
    if (!m_database.removeMovieByPath(m_transfer.finalPath)
        || !m_database.removeTransfer(m_transfer.torrentHash, m_transfer.fileIndex)) {
        m_error = m_database.lastError();
        emit stateChanged();
        return;
    }
    const QString removedHash = m_transfer.torrentHash;
    m_transfer = {};
    m_sourceUrl.clear();
    m_stateLabel.clear();
    m_error.clear();
    refreshDiskSpace();
    m_library.refresh();
    emit torrentRemovalRequested(removedHash);
    emit stateChanged();
}

bool DownloadManager::playMatchingRelease(const QString &title, const QString &magnetUrl)
{
    const QString hash = magnetInfoHash(magnetUrl);
    if (hash.isEmpty()) return false;
    if (!hasTransfer() || m_transfer.torrentHash.compare(hash, Qt::CaseInsensitive) != 0) {
        const std::optional<LibraryTransfer> stored = m_database.latestTransfer();
        if (!stored || stored->torrentHash.compare(hash, Qt::CaseInsensitive) != 0) return false;
        m_transfer = *stored;
    }
    const QFileInfo partial(m_transfer.partialPath);
    m_transfer.bytesWritten = partial.isFile() ? partial.size()
        : (QFileInfo(m_transfer.finalPath).isFile() ? m_transfer.expectedSize : 0);
    refreshDiskSpace();
    if (!playable()) return false;
    if (!title.isEmpty()) m_transfer.title = title;
    play();
    return true;
}

void DownloadManager::loadPending()
{
    const std::optional<LibraryTransfer> stored = m_database.latestTransfer();
    if (!stored) return;
    m_transfer = *stored;
    const QFileInfo partial(m_transfer.partialPath);
    m_transfer.bytesWritten = partial.isFile() ? partial.size() : 0;
    refreshDiskSpace();
    if (QFileInfo(m_transfer.finalPath).isFile()
        && QFileInfo(m_transfer.finalPath).size() == m_transfer.expectedSize) {
        m_transfer.bytesWritten = m_transfer.expectedSize;
        refreshDiskSpace();
        persist(QStringLiteral("completed"));
        m_stateLabel = tr("Saved to library");
        m_library.refresh();
        return;
    }
    if (partial.isFile() && partial.size() == m_transfer.expectedSize) {
        completeTransfer();
        return;
    }
    m_stateLabel = tr("Saved download waiting for VPN protection");
}

void DownloadManager::startRequest()
{
    if (!m_networkReady || m_sourceUrl.isEmpty() || m_reply) return;
    if (!QDir().mkpath(QFileInfo(m_transfer.partialPath).absolutePath())) {
        fail(tr("Could not create the movie library folder"));
        return;
    }
    m_requestOffset = QFileInfo(m_transfer.partialPath).isFile()
        ? QFileInfo(m_transfer.partialPath).size() : 0;
    m_transfer.bytesWritten = m_requestOffset;
    if (m_requestOffset == m_transfer.expectedSize) {
        completeTransfer();
        return;
    }
    if (!ensureDiskSpace()) return;
    m_file.setFileName(m_transfer.partialPath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        fail(tr("Could not open the partial movie file: %1").arg(m_file.errorString()));
        return;
    }
    m_requestOffset = m_file.size();
    QNetworkRequest request(m_sourceUrl);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                         QNetworkRequest::AlwaysNetwork);
    if (m_requestOffset > 0) {
        request.setRawHeader("Range", QByteArrayLiteral("bytes=")
            + QByteArray::number(m_requestOffset) + QByteArrayLiteral("-"));
    }
    m_reply = m_network.get(request);
    m_headersValidated = false;
    m_active = true;
    m_stateLabel = m_requestOffset > 0 ? tr("Resuming download…") : tr("Saving movie…");
    m_persistTimer.restart();
    connect(m_reply, &QNetworkReply::readyRead, this, &DownloadManager::writeAvailable);
    connect(m_reply, &QNetworkReply::finished, this, &DownloadManager::finishRequest);
    emit stateChanged();
}

bool DownloadManager::validateResponse()
{
    if (m_headersValidated) return true;
    if (!m_reply) return false;
    const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (m_requestOffset > 0) {
        const QByteArray expected = QByteArrayLiteral("bytes ") + QByteArray::number(m_requestOffset)
            + QByteArrayLiteral("-");
        if (status != 206 || !m_reply->rawHeader("Content-Range").startsWith(expected)) {
            fail(tr("TorrServer did not honor the resume byte range"));
            return false;
        }
    } else if (status != 200 && status != 206) {
        fail(tr("TorrServer returned HTTP %1 while saving the movie").arg(status));
        return false;
    }
    m_headersValidated = true;
    return true;
}

void DownloadManager::writeAvailable()
{
    if (!validateResponse() || !m_reply || !m_file.isOpen()) return;
    const QByteArray data = m_reply->readAll();
    if (!data.isEmpty() && m_file.write(data) != data.size()) {
        fail(m_file.error() == QFileDevice::ResourceError
                 ? tr("The movie library ran out of disk space while saving the movie")
                 : tr("Could not write the movie: %1").arg(m_file.errorString()));
        return;
    }
    m_transfer.bytesWritten = m_file.size();
    refreshDiskSpace();
    if (!m_diskSpaceReady) {
        fail(tr("The movie library no longer has enough free space to finish safely"));
        return;
    }
    if (m_transfer.bytesWritten > m_transfer.expectedSize) {
        fail(tr("TorrServer sent more data than the selected video contains"));
        return;
    }
    if (!m_persistTimer.isValid() || m_persistTimer.elapsed() >= 1'000) {
        if (!persist(QStringLiteral("downloading"))) {
            const QString databaseError = m_error;
            fail(databaseError);
            return;
        }
        m_persistTimer.restart();
    }
    emit stateChanged();
}

void DownloadManager::finishRequest()
{
    if (!m_reply) return;
    QNetworkReply *reply = m_reply;
    if (reply->bytesAvailable() > 0) writeAvailable();
    if (m_reply != reply) return;
    m_reply = nullptr;
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkMessage = reply->errorString();
    reply->deleteLater();
    if (m_file.isOpen()) {
        m_file.flush();
        if (m_file.handle() >= 0) ::fsync(m_file.handle());
        m_file.close();
    }
    m_active = false;
    m_transfer.bytesWritten = QFileInfo(m_transfer.partialPath).size();
    if (networkError != QNetworkReply::NoError) {
        if (!m_pausing) fail(tr("Movie download paused: %1").arg(networkMessage));
        return;
    }
    if (m_transfer.bytesWritten != m_transfer.expectedSize) {
        fail(tr("Movie download ended at %1 of %2 bytes")
                 .arg(m_transfer.bytesWritten).arg(m_transfer.expectedSize));
        return;
    }
    completeTransfer();
}

void DownloadManager::completeTransfer()
{
    if (m_file.isOpen()) {
        m_file.flush();
        if (m_file.handle() >= 0) ::fsync(m_file.handle());
        m_file.close();
    }
    if (QFileInfo::exists(m_transfer.partialPath)) {
        if (QFileInfo::exists(m_transfer.finalPath)) {
            if (QFileInfo(m_transfer.finalPath).size() != m_transfer.expectedSize) {
                fail(tr("A different file already exists at the final movie path"));
                return;
            }
            QFile::remove(m_transfer.partialPath);
        } else if (!QFile::rename(m_transfer.partialPath, m_transfer.finalPath)) {
            fail(tr("Could not finalize the downloaded movie"));
            return;
        }
    }
    const QByteArray directoryPath = QFileInfo(m_transfer.finalPath).absolutePath().toLocal8Bit();
    const int directoryFd = ::open(directoryPath.constData(), O_RDONLY | O_DIRECTORY);
    if (directoryFd >= 0) {
        ::fsync(directoryFd);
        ::close(directoryFd);
    }
    m_transfer.bytesWritten = m_transfer.expectedSize;
    refreshDiskSpace();
    const bool stateSaved = persist(QStringLiteral("completed"));
    m_active = false;
    m_stateLabel = tr("Saved to library");
    if (stateSaved) m_error.clear();
    m_library.refresh();
    emit stateChanged();
}

bool DownloadManager::persist(const QString &state)
{
    m_transfer.state = state;
    if (m_database.saveTransfer(m_transfer)) return true;
    m_error = m_database.lastError();
    return false;
}

void DownloadManager::refreshDiskSpace()
{
    const QString path = m_transfer.partialPath.isEmpty()
        ? m_library.directory() : QFileInfo(m_transfer.partialPath).absolutePath();
    const QStorageInfo storage(path);
    m_availableBytes = storage.isValid() && storage.isReady()
        ? qMax<qint64>(0, storage.bytesAvailable()) : 0;
    m_bytesRemaining = qMax<qint64>(0, m_transfer.expectedSize - m_transfer.bytesWritten);
    m_diskSpaceReady = m_bytesRemaining == 0
        || (storage.isValid() && storage.isReady()
            && m_availableBytes >= m_bytesRemaining
            && m_availableBytes - m_bytesRemaining >= DiskSafetyMargin);
}

bool DownloadManager::ensureDiskSpace()
{
    refreshDiskSpace();
    if (m_diskSpaceReady) return true;
    persist(QStringLiteral("paused"));
    m_active = false;
    m_stateLabel = tr("Waiting for disk space");
    m_error = tr("Not enough free space in the movie library. %1 GiB remains to be saved "
                 "and Dostflix keeps a 0.50 GiB safety margin, but only %2 GiB is available.")
                  .arg(static_cast<double>(m_bytesRemaining) / 1073741824.0, 0, 'f', 2)
                  .arg(static_cast<double>(m_availableBytes) / 1073741824.0, 0, 'f', 2);
    emit stateChanged();
    return false;
}

void DownloadManager::fail(QString error)
{
    if (m_reply) {
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
    if (m_file.isOpen()) {
        m_file.flush();
        m_file.close();
    }
    m_transfer.bytesWritten = QFileInfo(m_transfer.partialPath).size();
    refreshDiskSpace();
    persist(QStringLiteral("paused"));
    m_active = false;
    m_error = std::move(error);
    m_stateLabel = tr("Download paused");
    emit stateChanged();
}

QString DownloadManager::chooseFinalPath(const QString &fileName,
                                         const QString &torrentHash) const
{
    QString safeName = QFileInfo(fileName).fileName();
    if (safeName.isEmpty()) safeName = QStringLiteral("movie.mkv");
    QString result = QDir(m_library.directory()).filePath(safeName);
    if (QFileInfo::exists(result)) {
        const QFileInfo info(safeName);
        const QString suffix = info.suffix().isEmpty()
            ? QString() : QStringLiteral(".") + info.suffix();
        result = QDir(m_library.directory()).filePath(
            info.completeBaseName() + QStringLiteral(" (") + torrentHash.left(8)
            + QStringLiteral(")") + suffix);
    }
    return result;
}

bool DownloadManager::pathsAreSafe() const
{
    const QString root = QDir(m_library.directory()).absolutePath() + QLatin1Char('/');
    const auto safe = [&root](const QString &path) {
        const QFileInfo info(path);
        return !path.isEmpty() && info.absoluteFilePath().startsWith(root) && !info.isSymLink();
    };
    return safe(m_transfer.partialPath) && safe(m_transfer.finalPath);
}
