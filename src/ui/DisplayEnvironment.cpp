#include "ui/DisplayEnvironment.h"

#include <QString>

namespace {

bool containsGamescope(const QProcessEnvironment &environment, const QString &name)
{
    return environment.value(name).contains(QStringLiteral("gamescope"), Qt::CaseInsensitive);
}

bool enabled(const QProcessEnvironment &environment, const QString &name)
{
    if (!environment.contains(name)) return false;
    const QString value = environment.value(name).trimmed().toLower();
    return value.isEmpty() || (value != QStringLiteral("0")
                               && value != QStringLiteral("false")
                               && value != QStringLiteral("no"));
}

}

bool DisplayEnvironment::isGamescopeSession(const QProcessEnvironment &environment)
{
    return enabled(environment, QStringLiteral("STEAM_GAMESCOPE_SESSION"))
        || environment.contains(QStringLiteral("GAMESCOPE_WAYLAND_DISPLAY"))
        || containsGamescope(environment, QStringLiteral("XDG_CURRENT_DESKTOP"))
        || containsGamescope(environment, QStringLiteral("XDG_SESSION_DESKTOP"))
        || containsGamescope(environment, QStringLiteral("DESKTOP_SESSION"));
}
