#pragma once

#include <QString>
#include <QtGlobal>

struct Movie final
{
    QString id;
    QString title;
    int year = 0;
    QString posterUrl;
    QString quality;
    int seederCount = 0;
    qint64 sizeBytes = 0;
    QString sourceLabel;
    QString downloadUrl;
    QString magnetUrl;
};
