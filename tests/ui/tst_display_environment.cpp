#include "ui/DisplayEnvironment.h"

#include <QtTest>

class DisplayEnvironmentTest final : public QObject
{
    Q_OBJECT

private slots:
    void detectsSteamGamescopeSession()
    {
        QProcessEnvironment environment;
        environment.insert(QStringLiteral("STEAM_GAMESCOPE_SESSION"), QStringLiteral("1"));
        QVERIFY(DisplayEnvironment::isGamescopeSession(environment));
    }

    void detectsGamescopeDesktopNames()
    {
        QProcessEnvironment environment;
        environment.insert(QStringLiteral("XDG_CURRENT_DESKTOP"),
                           QStringLiteral("gamescope:KDE"));
        QVERIFY(DisplayEnvironment::isGamescopeSession(environment));
    }

    void ignoresDisabledOrNormalDesktopSession()
    {
        QProcessEnvironment environment;
        environment.insert(QStringLiteral("STEAM_GAMESCOPE_SESSION"), QStringLiteral("0"));
        environment.insert(QStringLiteral("XDG_CURRENT_DESKTOP"), QStringLiteral("KDE"));
        QVERIFY(!DisplayEnvironment::isGamescopeSession(environment));
    }
};

QTEST_GUILESS_MAIN(DisplayEnvironmentTest)
#include "tst_display_environment.moc"
