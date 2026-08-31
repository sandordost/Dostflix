#pragma once

#include "providers/ProviderListModel.h"

#include <QObject>

class AppSettings;
class SecretStore;

class ProviderManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ProviderListModel *model READ model CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(bool hasTmdbToken READ hasTmdbToken NOTIFY tmdbTokenChanged)

public:
    ProviderManager(AppSettings &settings, SecretStore &secrets, QObject *parent = nullptr);
    [[nodiscard]] ProviderListModel *model();
    [[nodiscard]] QString errorMessage() const;
    [[nodiscard]] bool hasTmdbToken() const;
    [[nodiscard]] QString tmdbToken() const;
    Q_INVOKABLE bool addProvider(const QString &name, const QString &kind,
                                 const QString &endpoint, const QString &apiKey);
    Q_INVOKABLE void removeProvider(int row);
    Q_INVOKABLE bool saveTmdbToken(const QString &token);
    Q_INVOKABLE void clearTmdbToken();

signals:
    void errorChanged();
    void tmdbTokenChanged();

private:
    void setError(QString error);
    AppSettings &m_settings;
    SecretStore &m_secrets;
    ProviderListModel m_model;
    QString m_error;
    QString m_tmdbToken;
};
