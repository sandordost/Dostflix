#include "providers/ProviderManager.h"

#include "app/AppSettings.h"
#include "providers/SecretStore.h"

#include <QUuid>

ProviderManager::ProviderManager(AppSettings &settings, SecretStore &secrets, QObject *parent)
    : QObject(parent), m_settings(settings), m_secrets(secrets)
{
    m_model.replace(settings.providers());
    QString ignored;
    m_tmdbToken = m_secrets.load(QStringLiteral("metadata-tmdb"), &ignored);
}

ProviderListModel *ProviderManager::model() { return &m_model; }
QString ProviderManager::errorMessage() const { return m_error; }
bool ProviderManager::hasTmdbToken() const { return !m_tmdbToken.isEmpty(); }
QString ProviderManager::tmdbToken() const { return m_tmdbToken; }

bool ProviderManager::addProvider(const QString &name, const QString &kind,
                                  const QString &endpoint, const QString &apiKey)
{
    const QUrl url(endpoint.trimmed());
    if (name.trimmed().isEmpty() || !url.isValid() || url.host().isEmpty()
        || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        setError(QStringLiteral("Enter a name and a valid HTTP(S) endpoint"));
        return false;
    }
    ProviderConfig provider{QUuid::createUuid().toString(QUuid::WithoutBraces), name.trimmed(),
        kind.compare(QStringLiteral("Prowlarr"), Qt::CaseInsensitive) == 0
            ? ProviderKind::Prowlarr : ProviderKind::Torznab, url, true};
    if (!apiKey.isEmpty()) {
        QString error;
        if (!m_secrets.store(provider.id, apiKey, &error)) {
            setError(error);
            return false;
        }
    }
    QList<ProviderConfig> providers = m_model.providers();
    providers.push_back(provider);
    m_settings.setProviders(providers);
    m_model.replace(std::move(providers));
    setError({});
    return true;
}

void ProviderManager::removeProvider(int row)
{
    QList<ProviderConfig> providers = m_model.providers();
    if (row < 0 || row >= providers.size()) return;
    const QString id = providers.at(row).id;
    providers.removeAt(row);
    m_settings.setProviders(providers);
    m_model.replace(std::move(providers));
    QString ignored;
    m_secrets.remove(id, &ignored);
}

bool ProviderManager::saveTmdbToken(const QString &token)
{
    const QString trimmed = token.trimmed();
    if (trimmed.isEmpty()) {
        setError(tr("Enter a TMDB API Read Access Token"));
        return false;
    }
    QString error;
    if (!m_secrets.store(QStringLiteral("metadata-tmdb"), trimmed, &error)) {
        setError(error);
        return false;
    }
    m_tmdbToken = trimmed;
    setError({});
    emit tmdbTokenChanged();
    return true;
}

void ProviderManager::clearTmdbToken()
{
    QString error;
    if (!m_secrets.remove(QStringLiteral("metadata-tmdb"), &error)) {
        setError(error);
        return;
    }
    m_tmdbToken.clear();
    setError({});
    emit tmdbTokenChanged();
}

void ProviderManager::setError(QString error)
{
    if (m_error == error) return;
    m_error = std::move(error);
    emit errorChanged();
}
