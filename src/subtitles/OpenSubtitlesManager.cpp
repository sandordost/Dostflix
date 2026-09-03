#include "subtitles/OpenSubtitlesManager.h"

#include "app/AppSettings.h"
#include "providers/SecretStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>
#include <QtEndian>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QUrlQuery>
#include <algorithm>
#include <utility>

namespace {
constexpr auto CredentialId = "subtitles-opensubtitles";

bool successful(const QNetworkReply *reply)
{
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return reply->error() == QNetworkReply::NoError && status >= 200 && status < 300;
}
}

OpenSubtitlesManager::OpenSubtitlesManager(AppSettings &settings, SecretStore &secrets,
                                           QString dataDir, QUrl apiBase, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_secrets(secrets)
    , m_dataDir(std::move(dataDir))
    , m_apiBase(std::move(apiBase))
    , m_network(new QNetworkAccessManager(this))
{
    m_preferredLanguages = normalizeLanguages(m_settings.subtitleLanguages());
    if (m_preferredLanguages.isEmpty()) m_preferredLanguages = QStringLiteral("nl,en");
    QString ignored;
    const QJsonObject stored = QJsonDocument::fromJson(
        m_secrets.load(QString::fromLatin1(CredentialId), &ignored).toUtf8()).object();
    m_apiKey = stored.value(QStringLiteral("apiKey")).toString();
    m_username = stored.value(QStringLiteral("username")).toString();
    m_password = stored.value(QStringLiteral("password")).toString();
}

OpenSubtitlesManager::~OpenSubtitlesManager() { cancel(); }
bool OpenSubtitlesManager::configured() const
{ return !m_apiKey.isEmpty() && !m_username.isEmpty() && !m_password.isEmpty(); }
QString OpenSubtitlesManager::username() const { return m_username; }
QString OpenSubtitlesManager::preferredLanguages() const { return m_preferredLanguages; }
bool OpenSubtitlesManager::networkReady() const { return m_networkReady; }
bool OpenSubtitlesManager::busy() const { return !m_reply.isNull(); }
QString OpenSubtitlesManager::statusLabel() const { return m_status; }
QString OpenSubtitlesManager::errorMessage() const { return m_error; }
QVariantList OpenSubtitlesManager::results() const { return m_results; }

void OpenSubtitlesManager::setNetworkReady(bool ready)
{
    if (m_networkReady == ready) return;
    m_networkReady = ready;
    if (!ready) {
        cancel();
        m_token.clear();
        m_loginApiBase.clear();
        m_status = tr("Waiting for VPN protection");
    } else {
        m_status.clear();
    }
    emit stateChanged();
}

bool OpenSubtitlesManager::saveCredentials(const QString &apiKey, const QString &username,
                                           const QString &password)
{
    if (apiKey.trimmed().isEmpty() || username.trimmed().isEmpty() || password.isEmpty()) {
        setError(tr("Enter an OpenSubtitles API key, username, and password"));
        return false;
    }
    const QJsonObject credentials{{QStringLiteral("apiKey"), apiKey.trimmed()},
                                  {QStringLiteral("username"), username.trimmed()},
                                  {QStringLiteral("password"), password}};
    QString error;
    if (!m_secrets.store(QString::fromLatin1(CredentialId),
                         QString::fromUtf8(QJsonDocument(credentials).toJson(QJsonDocument::Compact)),
                         &error)) {
        setError(error);
        return false;
    }
    m_apiKey = apiKey.trimmed();
    m_username = username.trimmed();
    m_password = password;
    m_token.clear();
    m_loginApiBase.clear();
    setError({});
    emit configurationChanged();
    return true;
}

void OpenSubtitlesManager::clearCredentials()
{
    QString error;
    if (!m_secrets.remove(QString::fromLatin1(CredentialId), &error)) {
        setError(error);
        return;
    }
    cancel();
    m_apiKey.clear(); m_username.clear(); m_password.clear(); m_token.clear();
    m_loginApiBase.clear();
    m_results.clear();
    setError({});
    emit configurationChanged();
    emit resultsChanged();
}

bool OpenSubtitlesManager::setPreferredLanguages(const QString &languages)
{
    const QString normalized = normalizeLanguages(languages);
    if (normalized.isEmpty()) {
        setError(tr("Enter one or more two- or three-letter language codes"));
        return false;
    }
    if (m_preferredLanguages == normalized) return true;
    m_preferredLanguages = normalized;
    m_settings.setSubtitleLanguages(normalized);
    setError({});
    emit configurationChanged();
    return true;
}

void OpenSubtitlesManager::setMediaContext(const QUrl &videoUrl, const QString &imdbId)
{
    m_videoPath = videoUrl.isLocalFile() ? videoUrl.toLocalFile() : QString{};
    m_imdbId = imdbId.trimmed();
}

void OpenSubtitlesManager::search(const QString &query, const QString &languages)
{
    if (!m_networkReady) { setError(tr("VPN protection is required to search subtitles")); return; }
    if (!configured()) { setError(tr("Configure OpenSubtitles in Settings first")); return; }
    if (query.trimmed().isEmpty()) { setError(tr("Enter a movie or release title")); return; }
    cancel();
    m_query = query.trimmed();
    m_languages = normalizeLanguages(languages.trimmed().isEmpty()
                                         ? m_preferredLanguages : languages);
    if (m_languages.isEmpty()) {
        setError(tr("Enter valid subtitle language codes"));
        return;
    }
    m_pendingAction = PendingAction::Search;
    m_status = tr("Signing in to OpenSubtitles…");
    setError({});
    authenticate();
}

void OpenSubtitlesManager::download(int row)
{
    if (!m_networkReady) { setError(tr("VPN protection is required to download subtitles")); return; }
    if (row < 0 || row >= m_results.size()) return;
    cancel();
    m_pendingRow = row;
    m_pendingAction = PendingAction::Download;
    m_status = tr("Preparing subtitle download…");
    setError({});
    authenticate();
}

void OpenSubtitlesManager::authenticate()
{
    if (!m_token.isEmpty()) { continuePendingAction(); return; }
    const QJsonObject body{{QStringLiteral("username"), m_username},
                           {QStringLiteral("password"), m_password}};
    QNetworkReply *reply = sendJson(QStringLiteral("login"), QByteArrayLiteral("POST"),
                                    QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        if (!successful(reply)) {
            setError(tr("OpenSubtitles login failed: %1").arg(responseError(reply)));
        } else {
            const QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
            m_token = response.value(QStringLiteral("token")).toString();
            applyLoginBaseUrl(response.value(QStringLiteral("base_url")).toString());
            if (m_token.isEmpty()) setError(tr("OpenSubtitles returned no login token"));
            else continuePendingAction();
        }
        reply->deleteLater();
        emit stateChanged();
    });
}

void OpenSubtitlesManager::continuePendingAction()
{
    if (m_pendingAction == PendingAction::Search) performSearch();
    else if (m_pendingAction == PendingAction::Download) requestDownload();
}

void OpenSubtitlesManager::performSearch()
{
    m_status = tr("Searching OpenSubtitles…");
    QUrl url = authenticatedApiBase().resolved(QUrl(QStringLiteral("subtitles")));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("query"), m_query);
    query.addQueryItem(QStringLiteral("languages"), m_languages);
    QString numericImdb = m_imdbId;
    if (numericImdb.startsWith(QStringLiteral("tt"), Qt::CaseInsensitive)) numericImdb.remove(0, 2);
    bool validImdb = false;
    numericImdb.toLongLong(&validImdb);
    if (validImdb) query.addQueryItem(QStringLiteral("imdb_id"), numericImdb);
    qint64 fileSize = 0;
    const QString hash = movieHash(m_videoPath, &fileSize);
    if (!hash.isEmpty()) {
        query.addQueryItem(QStringLiteral("moviehash"), hash);
        query.addQueryItem(QStringLiteral("moviebytesize"), QString::number(fileSize));
    }
    query.addQueryItem(QStringLiteral("order_by"), QStringLiteral("download_count"));
    query.addQueryItem(QStringLiteral("order_direction"), QStringLiteral("desc"));
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader("Api-Key", m_apiKey.toUtf8());
    request.setRawHeader("User-Agent", "Dostflix v1.0.0");
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(15'000);
    QNetworkReply *reply = m_network->get(request);
    m_reply = reply;
    emit stateChanged();
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        if (!successful(reply)) {
            setError(tr("Subtitle search failed: %1").arg(responseError(reply)));
        } else {
            QVariantList results;
            const QJsonArray data = QJsonDocument::fromJson(reply->readAll()).object()
                                        .value(QStringLiteral("data")).toArray();
            for (const QJsonValue &value : data) {
                const QJsonObject attributes = value.toObject().value(QStringLiteral("attributes")).toObject();
                const QJsonArray files = attributes.value(QStringLiteral("files")).toArray();
                if (files.isEmpty()) continue;
                const QJsonObject file = files.first().toObject();
                const int fileId = file.value(QStringLiteral("file_id")).toInt();
                if (fileId <= 0) continue;
                QString release = attributes.value(QStringLiteral("release")).toString();
                if (release.isEmpty()) release = file.value(QStringLiteral("file_name")).toString();
                results.append(QVariantMap{
                    {QStringLiteral("fileId"), fileId},
                    {QStringLiteral("language"), attributes.value(QStringLiteral("language")).toString()},
                    {QStringLiteral("release"), release},
                    {QStringLiteral("downloads"), attributes.value(QStringLiteral("download_count")).toInt()},
                    {QStringLiteral("hearingImpaired"), attributes.value(QStringLiteral("hearing_impaired")).toBool()},
                    {QStringLiteral("trusted"), attributes.value(QStringLiteral("from_trusted")).toBool()},
                    {QStringLiteral("fileName"), file.value(QStringLiteral("file_name")).toString()},
                });
                if (results.size() >= 50) break;
            }
            m_results = results;
            m_status = results.isEmpty() ? tr("No subtitles found")
                                         : tr("%1 subtitle matches").arg(results.size());
            m_error.clear();
            emit resultsChanged();
        }
        m_pendingAction = PendingAction::None;
        reply->deleteLater();
        emit stateChanged();
    });
}

void OpenSubtitlesManager::requestDownload()
{
    if (m_pendingRow < 0 || m_pendingRow >= m_results.size()) return;
    const int fileId = m_results.at(m_pendingRow).toMap().value(QStringLiteral("fileId")).toInt();
    QNetworkReply *reply = sendJson(
        QStringLiteral("download"), QByteArrayLiteral("POST"),
        QJsonDocument(QJsonObject{{QStringLiteral("file_id"), fileId}}).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        if (!successful(reply)) {
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401)
                m_token.clear();
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == 503) {
                setError(tr("OpenSubtitles accepted the login, but its download service is unavailable (503). Try again later."));
            } else {
                setError(tr("Could not prepare subtitle download: %1").arg(responseError(reply)));
            }
            m_pendingAction = PendingAction::None;
            m_pendingRow = -1;
            reply->deleteLater();
            emit stateChanged();
            return;
        }
        const QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
        const QUrl link(response.value(QStringLiteral("link")).toString());
        const QString fileName = response.value(QStringLiteral("file_name")).toString();
        reply->deleteLater();
        if (!link.isValid() || (link.scheme() != QStringLiteral("https")
                                && link.scheme() != QStringLiteral("http"))) {
            setError(tr("OpenSubtitles returned an invalid download link"));
            m_pendingAction = PendingAction::None;
            m_pendingRow = -1;
            emit stateChanged();
            return;
        }
        fetchSubtitle(link, fileName);
    });
}

void OpenSubtitlesManager::fetchSubtitle(const QUrl &url, QString fileName)
{
    m_status = tr("Downloading subtitle…");
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Dostflix v1.0.0");
    request.setTransferTimeout(20'000);
    QNetworkReply *reply = m_network->get(request);
    m_reply = reply;
    emit stateChanged();
    connect(reply, &QNetworkReply::finished, this, [this, reply, fileName = std::move(fileName)] {
        if (m_reply != reply) return;
        m_reply = nullptr;
        if (!successful(reply)) {
            setError(tr("Subtitle download failed: %1").arg(responseError(reply)));
        } else {
            QString safeName = QFileInfo(fileName).fileName();
            const QString suffix = QFileInfo(safeName).suffix().toLower();
            if (safeName.isEmpty()) safeName = QStringLiteral("subtitle.srt");
            else if (suffix != QStringLiteral("srt") && suffix != QStringLiteral("ass")
                     && suffix != QStringLiteral("vtt")) safeName += QStringLiteral(".srt");
            QString directory = QDir(m_dataDir).filePath(QStringLiteral("subtitles"));
            if (!m_videoPath.isEmpty())
                directory = QFileInfo(m_videoPath).absolutePath();
            if (!QDir().mkpath(directory)) {
                setError(tr("Could not create the subtitle directory"));
            } else {
                if (!m_videoPath.isEmpty()) {
                    const QString language = m_pendingRow >= 0 && m_pendingRow < m_results.size()
                        ? m_results.at(m_pendingRow).toMap().value(QStringLiteral("language")).toString()
                        : QString{};
                    const QString extension = QFileInfo(safeName).suffix().toLower();
                    safeName = QFileInfo(m_videoPath).completeBaseName()
                        + (language.isEmpty() ? QString{} : QStringLiteral(".") + language)
                        + QStringLiteral(".") + extension;
                }
                const QString destination = QDir(directory).filePath(safeName);
                QSaveFile file(destination);
                if (!file.open(QIODevice::WriteOnly) || file.write(reply->readAll()) < 0 || !file.commit()) {
                    setError(tr("Could not save the downloaded subtitle"));
                } else {
                    m_status = tr("Subtitle loaded");
                    m_error.clear();
                    emit subtitleReady(QUrl::fromLocalFile(destination));
                }
            }
        }
        m_pendingAction = PendingAction::None;
        m_pendingRow = -1;
        reply->deleteLater();
        emit stateChanged();
    });
}

void OpenSubtitlesManager::cancel()
{
    clearReply();
    m_pendingAction = PendingAction::None;
    m_pendingRow = -1;
    emit stateChanged();
}

void OpenSubtitlesManager::setError(QString error)
{
    m_error = std::move(error);
    if (!m_error.isEmpty()) m_status.clear();
    emit stateChanged();
}

void OpenSubtitlesManager::clearReply()
{
    if (!m_reply) return;
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    reply->abort();
    reply->deleteLater();
}

QNetworkReply *OpenSubtitlesManager::sendJson(const QString &path, const QByteArray &method,
                                              const QByteArray &body)
{
    const QUrl base = path == QStringLiteral("login") ? m_apiBase : authenticatedApiBase();
    QNetworkRequest request(base.resolved(QUrl(path)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Api-Key", m_apiKey.toUtf8());
    request.setRawHeader("User-Agent", "Dostflix v1.0.0");
    request.setRawHeader("Accept", "application/json");
    if (!m_token.isEmpty()) request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + m_token.toUtf8());
    request.setTransferTimeout(15'000);
    QNetworkReply *reply = m_network->sendCustomRequest(request, method, body);
    m_reply = reply;
    emit stateChanged();
    return reply;
}

QUrl OpenSubtitlesManager::authenticatedApiBase() const
{
    return m_loginApiBase.isValid() && !m_loginApiBase.isEmpty()
        ? m_loginApiBase : m_apiBase;
}

void OpenSubtitlesManager::applyLoginBaseUrl(const QString &host)
{
    const QString normalized = host.trimmed().toLower();
    const bool official = normalized == QStringLiteral("api.opensubtitles.com")
        || normalized == QStringLiteral("vip-api.opensubtitles.com");
    const bool localTestRoute = QHostAddress(m_apiBase.host()).isLoopback()
        && QHostAddress(normalized).isLoopback();
    if (!official && normalized != m_apiBase.host().toLower() && !localTestRoute) return;
    QUrl routed = m_apiBase;
    routed.setHost(normalized);
    if (official) {
        routed.setScheme(QStringLiteral("https"));
        routed.setPort(-1);
        routed.setPath(QStringLiteral("/api/v1/"));
    }
    m_loginApiBase = routed;
}

QString OpenSubtitlesManager::responseError(QNetworkReply *reply) const
{
    const QByteArray body = reply->readAll();
    const QString message = QJsonDocument::fromJson(body).object().value(QStringLiteral("message")).toString();
    return message.isEmpty() ? reply->errorString() : message;
}

QString OpenSubtitlesManager::normalizeLanguages(const QString &languages)
{
    QStringList result;
    for (QString language : languages.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        language = language.trimmed().toLower();
        if ((language.size() != 2 && language.size() != 3)
            || std::any_of(language.cbegin(), language.cend(), [](QChar character) {
                   return character < QLatin1Char('a') || character > QLatin1Char('z');
               })) return {};
        if (!result.contains(language)) result.append(language);
    }
    return result.join(QLatin1Char(','));
}

QString OpenSubtitlesManager::movieHash(const QString &videoPath, qint64 *fileSize)
{
    if (fileSize) *fileSize = 0;
    QFile file(videoPath);
    if (!file.open(QIODevice::ReadOnly) || file.size() < 131072) return {};
    const qint64 size = file.size();
    quint64 hash = static_cast<quint64>(size);
    auto addBlock = [&file, &hash](qint64 offset) {
        if (!file.seek(offset)) return false;
        const QByteArray block = file.read(65536);
        if (block.size() != 65536) return false;
        for (qsizetype index = 0; index < block.size(); index += 8)
            hash += qFromLittleEndian<quint64>(block.constData() + index);
        return true;
    };
    if (!addBlock(0) || !addBlock(size - 65536)) return {};
    if (fileSize) *fileSize = size;
    return QStringLiteral("%1").arg(hash, 16, 16, QLatin1Char('0'));
}
