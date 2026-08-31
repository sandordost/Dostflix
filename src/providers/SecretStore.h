#pragma once

#include <QString>

class SecretStore
{
public:
    virtual ~SecretStore() = default;
    virtual bool store(const QString &providerId, const QString &secret, QString *error) = 0;
    virtual QString load(const QString &providerId, QString *error) = 0;
    virtual bool remove(const QString &providerId, QString *error) = 0;
};

class LibSecretStore final : public SecretStore
{
public:
    bool store(const QString &providerId, const QString &secret, QString *error) override;
    QString load(const QString &providerId, QString *error) override;
    bool remove(const QString &providerId, QString *error) override;
};
