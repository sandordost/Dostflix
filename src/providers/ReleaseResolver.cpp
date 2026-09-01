#include "providers/ReleaseResolver.h"

bool isMagnetUrl(const QString &value)
{
    return value.trimmed().startsWith(QStringLiteral("magnet:?"), Qt::CaseInsensitive);
}

ReleaseLocation resolveReleaseLocation(const QString &magnetField,
                                       const QString &downloadField)
{
    const QString magnet = magnetField.trimmed();
    const QString download = downloadField.trimmed();
    if (isMagnetUrl(magnet)) return {magnet, {}};
    if (isMagnetUrl(download)) return {download, {}};

    for (const QString &candidate : {download, magnet}) {
        const QUrl url(candidate);
        if (url.isValid()
            && (url.scheme().compare(QStringLiteral("http"), Qt::CaseInsensitive) == 0
                || url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0)) {
            return {{}, url};
        }
    }
    return {};
}
