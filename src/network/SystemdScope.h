#pragma once

#include <QString>

class SystemdScope final
{
public:
    [[nodiscard]] static bool enter(QString *error);
};
