#pragma once

#include <QString>

struct ParsedMovieFilename final
{
    QString title;
    int year = 0;
};

ParsedMovieFilename parseMovieFilename(const QString &path);
