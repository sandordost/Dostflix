#include "library/MovieFilenameParser.h"

#include <QFileInfo>
#include <QRegularExpression>

ParsedMovieFilename parseMovieFilename(const QString &path)
{
    QString name = QFileInfo(path).completeBaseName();
    name.replace(QRegularExpression(QStringLiteral("[._]+")), QStringLiteral(" "));
    static const QRegularExpression yearExpression(
        QStringLiteral("(?:^|[\\s(\\[])(19\\d{2}|20\\d{2})(?=[\\s)\\]]|$)"));
    QRegularExpressionMatch yearMatch;
    QRegularExpressionMatchIterator yearMatches = yearExpression.globalMatch(name);
    while (yearMatches.hasNext()) yearMatch = yearMatches.next();
    const int year = yearMatch.hasMatch() ? yearMatch.captured(1).toInt() : 0;
    if (yearMatch.hasMatch()) name = name.left(yearMatch.capturedStart()).trimmed();
    name.replace(QRegularExpression(QStringLiteral("\\[[^]]*\\]|\\([^)]*\\)")),
                 QStringLiteral(" "));
    if (!yearMatch.hasMatch()) {
        static const QRegularExpression releaseToken(QStringLiteral(
            "\\s(?:2160p|1080p|720p|480p|web[ .-]?dl|webrip|bluray|brrip|dvdrip|"
            "h\\.?26[45]|hevc|x26[45]|av1|remux|hdr|ddp?\\d|aac)(?:\\s.*)?$"),
            QRegularExpression::CaseInsensitiveOption);
        name.remove(releaseToken);
    }
    name.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    return {name.trimmed(), year};
}
