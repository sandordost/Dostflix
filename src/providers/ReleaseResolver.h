#pragma once

#include <QString>
#include <QUrl>

struct ReleaseLocation final
{
    QString magnetUrl;
    QUrl torrentUrl;
};

[[nodiscard]] ReleaseLocation resolveReleaseLocation(const QString &magnetField,
                                                     const QString &downloadField);
[[nodiscard]] bool isMagnetUrl(const QString &value);
