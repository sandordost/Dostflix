#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QProcess>
#include <QTimer>

class MovieListModel;
class ProviderManager;
class QNetworkReply;

class ProwlarrManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool installed READ installed CONSTANT)
    Q_PROPERTY(bool running READ running NOTIFY stateChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY stateChanged)
    Q_PROPERTY(QString stateLabel READ stateLabel NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(QString webUrl READ webUrl CONSTANT)
    Q_PROPERTY(bool searchBusy READ searchBusy NOTIFY searchStateChanged)
    Q_PROPERTY(QString searchError READ searchError NOTIFY searchStateChanged)
    Q_PROPERTY(bool releaseBusy READ releaseBusy NOTIFY releaseStateChanged)
    Q_PROPERTY(QString releaseError READ releaseError NOTIFY releaseStateChanged)

public:
    explicit ProwlarrManager(QString dataDir, MovieListModel &movieModel,
                             ProviderManager &providerManager,
                             QObject *parent = nullptr);

    [[nodiscard]] bool installed() const;
    [[nodiscard]] bool running() const;
    [[nodiscard]] bool ready() const;
    [[nodiscard]] QString stateLabel() const;
    [[nodiscard]] QString errorMessage() const;
    [[nodiscard]] QString webUrl() const;
    [[nodiscard]] QString apiKey() const;
    [[nodiscard]] bool searchBusy() const;
    [[nodiscard]] QString searchError() const;
    [[nodiscard]] bool releaseBusy() const;
    [[nodiscard]] QString releaseError() const;

    void setNetworkReady(bool ready);
    void shutdown();
    Q_INVOKABLE void openWebInterface();
    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE void prepareRelease(const QString &title, const QString &magnetUrl,
                                    const QString &downloadUrl);

signals:
    void stateChanged();
    void searchStateChanged();
    void releaseStateChanged();
    void releasePrepared(const QString &title, const QString &magnetUrl,
                         const QByteArray &torrentData);

private:
    bool ensureConfig();
    void start();
    void stop();
    void probe();
    void fetchMetadata(const QString &query);
    void fetchRelease(const QString &title, const QUrl &url, int redirectsRemaining);
    void setError(QString error);

    static constexpr auto Executable = "/usr/lib/prowlarr/bin/Prowlarr";
    QString m_dataDir;
    MovieListModel &m_movieModel;
    ProviderManager &m_providerManager;
    QString m_apiKey;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_searchReply;
    QPointer<QNetworkReply> m_metadataReply;
    QPointer<QNetworkReply> m_releaseReply;
    QProcess m_process;
    QTimer m_probeTimer;
    bool m_networkReady = false;
    bool m_ready = false;
    QString m_error;
    QString m_searchError;
    QString m_searchQuery;
    QString m_releaseError;
};
