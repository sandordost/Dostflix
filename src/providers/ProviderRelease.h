#pragma once

#include <QDateTime>
#include <QString>
#include <QUrl>

struct ProviderRelease
{
    QString title;
    QString sourceLabel;
    QUrl downloadUrl;
    QString magnetUrl;
    qint64 sizeBytes = 0;
    int seeders = 0;
    int leechers = 0;
    QDateTime publishedAt;
    QString category;
};
