#include "providers/TorznabParser.h"

#include <QXmlStreamReader>

QList<ProviderRelease> TorznabParser::parse(const QByteArray &xml,
                                            const QString &sourceLabel,
                                            QString *error)
{
    QXmlStreamReader reader(xml);
    QList<ProviderRelease> releases;
    ProviderRelease current;
    bool inItem = false;
    int peers = 0;
    bool hasLeechers = false;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == QStringLiteral("item")) {
            current = {};
            current.sourceLabel = sourceLabel;
            peers = 0;
            hasLeechers = false;
            inItem = true;
        } else if (reader.isEndElement() && reader.name() == QStringLiteral("item")) {
            if (!hasLeechers) current.leechers = qMax(0, peers - current.seeders);
            if (!current.title.isEmpty()
                && (current.downloadUrl.isValid() || !current.magnetUrl.isEmpty())) {
                releases.push_back(current);
            }
            inItem = false;
        } else if (inItem && reader.isStartElement()) {
            if (reader.name() == QStringLiteral("title")) {
                current.title = reader.readElementText().trimmed();
            } else if (reader.name() == QStringLiteral("link")) {
                current.downloadUrl = QUrl(reader.readElementText().trimmed());
            } else if (reader.name() == QStringLiteral("pubDate")) {
                current.publishedAt = QDateTime::fromString(
                    reader.readElementText().trimmed(), Qt::RFC2822Date);
            } else if (reader.name() == QStringLiteral("enclosure")) {
                const auto attributes = reader.attributes();
                current.downloadUrl = QUrl(attributes.value(QStringLiteral("url")).toString());
                current.sizeBytes = attributes.value(QStringLiteral("length")).toLongLong();
            } else if (reader.name() == QStringLiteral("attr")) {
                const auto attributes = reader.attributes();
                const QString name = attributes.value(QStringLiteral("name")).toString();
                const QString value = attributes.value(QStringLiteral("value")).toString();
                if (name == QStringLiteral("seeders")) current.seeders = value.toInt();
                else if (name == QStringLiteral("peers")) peers = value.toInt();
                else if (name == QStringLiteral("leechers")) {
                    current.leechers = value.toInt();
                    hasLeechers = true;
                } else if (name == QStringLiteral("size")) current.sizeBytes = value.toLongLong();
                else if (name == QStringLiteral("category")) current.category = value;
                else if (name == QStringLiteral("magneturl")) current.magnetUrl = value;
            }
        }
    }

    if (reader.hasError()) {
        if (error) *error = reader.errorString();
        return {};
    }
    if (error) error->clear();
    return releases;
}
