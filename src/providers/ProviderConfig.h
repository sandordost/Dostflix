#pragma once

#include <QString>
#include <QUrl>

enum class ProviderKind { Prowlarr, Torznab };

struct ProviderConfig
{
    QString id;
    QString name;
    ProviderKind kind = ProviderKind::Torznab;
    QUrl endpoint;
    bool enabled = true;

    friend bool operator==(const ProviderConfig &, const ProviderConfig &) = default;
};
