#include "providers/ProwlarrManager.h"

#include "movies/MovieListModel.h"

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

ProwlarrManager::ProwlarrManager(QString dataDir, MovieListModel &movieModel, QObject *parent)
    : QObject(parent), m_dataDir(std::move(dataDir)), m_movieModel(movieModel)
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
bool ProwlarrManager::searchBusy() const { return !m_searchReply.isNull(); }
QString ProwlarrManager::searchError() const { return m_searchError; }

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
    if (!m_ready || trimmed.isEmpty() || searchBusy()) return;

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

    connect(m_searchReply, &QNetworkReply::finished, this, [this] {
        QNetworkReply *reply = m_searchReply;
        if (reply == nullptr) return;
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
        emit searchStateChanged();
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
    m_process.start();
}

void ProwlarrManager::stop()
{
    if (m_searchReply) m_searchReply->abort();
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
