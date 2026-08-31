#pragma once

#include "providers/ProviderRelease.h"

#include <QList>

class TorznabParser final
{
public:
    [[nodiscard]] static QList<ProviderRelease> parse(const QByteArray &xml,
                                                      const QString &sourceLabel,
                                                      QString *error);
};
