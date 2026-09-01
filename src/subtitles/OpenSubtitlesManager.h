#pragma once

#include <QObject>
#include <QPointer>
#include <QUrl>
#include <QVariantList>

class QNetworkAccessManager;
class QNetworkReply;
class SecretStore;

class OpenSubtitlesManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured NOTIFY configurationChanged)
    Q_PROPERTY(QString username READ username NOTIFY configurationChanged)
    Q_PROPERTY(bool networkReady READ networkReady NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString statusLabel READ statusLabel NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)

public:
    OpenSubtitlesManager(SecretStore &secrets, QString dataDir,
                         QUrl apiBase = QUrl(QStringLiteral("https://api.opensubtitles.com/api/v1/")),
                         QObject *parent = nullptr);
    ~OpenSubtitlesManager() override;

    bool configured() const;
    QString username() const;
    bool networkReady() const;
    bool busy() const;
    QString statusLabel() const;
    QString errorMessage() const;
    QVariantList results() const;

    void setNetworkReady(bool ready);
    Q_INVOKABLE bool saveCredentials(const QString &apiKey, const QString &username,
                                     const QString &password);
    Q_INVOKABLE void clearCredentials();
    Q_INVOKABLE void search(const QString &query, const QString &languages = QStringLiteral("nl,en"));
    Q_INVOKABLE void download(int row);
    Q_INVOKABLE void cancel();

signals:
    void configurationChanged();
    void stateChanged();
    void resultsChanged();
    void subtitleReady(const QUrl &fileUrl);

private:
    enum class PendingAction { None, Search, Download };
    void authenticate();
    void continuePendingAction();
    void performSearch();
    void requestDownload();
    void fetchSubtitle(const QUrl &url, QString fileName);
    void setError(QString error);
    void clearReply();
    QNetworkReply *sendJson(const QString &path, const QByteArray &method,
                            const QByteArray &body = {});
    QString responseError(QNetworkReply *reply) const;

    SecretStore &m_secrets;
    QString m_dataDir;
    QUrl m_apiBase;
    QNetworkAccessManager *m_network;
    QPointer<QNetworkReply> m_reply;
    QString m_apiKey;
    QString m_username;
    QString m_password;
    QString m_token;
    QString m_query;
    QString m_languages;
    QString m_status;
    QString m_error;
    QVariantList m_results;
    PendingAction m_pendingAction = PendingAction::None;
    int m_pendingRow = -1;
    bool m_networkReady = false;
};
