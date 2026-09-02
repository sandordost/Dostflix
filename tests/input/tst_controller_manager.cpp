#include "input/ControllerManager.h"

#include <QSignalSpy>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>

class ControllerManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void mapsCommonFaceAndShoulderButtons()
    {
        ControllerManager manager(nullptr, false);
        QSignalSpy spy(&manager, &ControllerManager::actionTriggered);

        manager.processButton(SDL_GAMEPAD_BUTTON_SOUTH, true);
        manager.processButton(SDL_GAMEPAD_BUTTON_EAST, true);
        manager.processButton(SDL_GAMEPAD_BUTTON_START, true);
        manager.processButton(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
        manager.processButton(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, true);
        manager.processButton(SDL_GAMEPAD_BUTTON_NORTH, true);
        manager.processButton(SDL_GAMEPAD_BUTTON_WEST, true);

        QCOMPARE(spy.size(), 7);
        QCOMPARE(spy.at(0).at(0).value<ControllerManager::Action>(), ControllerManager::Confirm);
        QCOMPARE(spy.at(1).at(0).value<ControllerManager::Action>(), ControllerManager::Back);
        QCOMPARE(spy.at(2).at(0).value<ControllerManager::Action>(), ControllerManager::PlayPause);
        QCOMPARE(spy.at(3).at(0).value<ControllerManager::Action>(), ControllerManager::PreviousPage);
        QCOMPARE(spy.at(4).at(0).value<ControllerManager::Action>(), ControllerManager::NextPage);
        QCOMPARE(spy.at(5).at(0).value<ControllerManager::Action>(), ControllerManager::ToggleFullscreen);
        QCOMPARE(spy.at(6).at(0).value<ControllerManager::Action>(), ControllerManager::OpenSubtitles);
    }

    void ignoresButtonRelease()
    {
        ControllerManager manager(nullptr, false);
        QSignalSpy spy(&manager, &ControllerManager::actionTriggered);
        manager.processButton(SDL_GAMEPAD_BUTTON_SOUTH, false);
        QCOMPARE(spy.size(), 0);
    }

    void supportsDpadAndAnalogDeadZone()
    {
        ControllerManager manager(nullptr, false);
        QSignalSpy spy(&manager, &ControllerManager::actionTriggered);

        manager.processButton(SDL_GAMEPAD_BUTTON_DPAD_DOWN, true);
        manager.processButton(SDL_GAMEPAD_BUTTON_DPAD_DOWN, false);
        manager.processAxis(SDL_GAMEPAD_AXIS_LEFTX, 15'000);
        manager.processAxis(SDL_GAMEPAD_AXIS_LEFTX, 17'000);
        manager.processAxis(SDL_GAMEPAD_AXIS_LEFTX, 12'000);
        manager.processAxis(SDL_GAMEPAD_AXIS_LEFTX, 9'000);
        manager.processAxis(SDL_GAMEPAD_AXIS_LEFTX, 17'000);

        QCOMPARE(spy.size(), 3);
        QCOMPARE(spy.at(0).at(0).value<ControllerManager::Action>(), ControllerManager::NavigateDown);
        QCOMPARE(spy.at(1).at(0).value<ControllerManager::Action>(), ControllerManager::NavigateRight);
        QCOMPARE(spy.at(2).at(0).value<ControllerManager::Action>(), ControllerManager::NavigateRight);
    }

    void doesNotDoubleTriggerWhenDpadAndStickOverlap()
    {
        ControllerManager manager(nullptr, false);
        QSignalSpy spy(&manager, &ControllerManager::actionTriggered);

        manager.processButton(SDL_GAMEPAD_BUTTON_DPAD_LEFT, true);
        manager.processAxis(SDL_GAMEPAD_AXIS_LEFTX, -20'000);
        manager.processButton(SDL_GAMEPAD_BUTTON_DPAD_LEFT, false);
        manager.processAxis(SDL_GAMEPAD_AXIS_LEFTX, 0);

        QCOMPARE(spy.size(), 1);
        QCOMPARE(spy.at(0).at(0).value<ControllerManager::Action>(), ControllerManager::NavigateLeft);
    }

    void mapsAnalogTriggersWithHysteresis()
    {
        ControllerManager manager(nullptr, false);
        QSignalSpy spy(&manager, &ControllerManager::actionTriggered);

        manager.processAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 17'000);
        manager.processAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 12'000);
        manager.processAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 9'000);
        manager.processAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 17'000);

        QCOMPARE(spy.size(), 2);
        QCOMPARE(spy.at(0).at(0).value<ControllerManager::Action>(), ControllerManager::PreviousPage);
        QCOMPARE(spy.at(1).at(0).value<ControllerManager::Action>(), ControllerManager::NextPage);
    }

    void movesFocusInTheRequestedVisualDirection()
    {
        ControllerManager manager(nullptr, false);
        QQuickWindow window;
        window.setGeometry(0, 0, 400, 300);

        QQuickItem current(window.contentItem());
        current.setPosition({20, 20});
        current.setSize({60, 40});
        current.setActiveFocusOnTab(true);

        QQuickItem right(window.contentItem());
        right.setPosition({140, 20});
        right.setSize({60, 40});
        right.setActiveFocusOnTab(true);

        QQuickItem down(window.contentItem());
        down.setPosition({20, 140});
        down.setSize({60, 40});
        down.setActiveFocusOnTab(true);

        window.show();
        window.requestActivate();
        QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
        QObject popupState(&window);
        popupState.setProperty("opened", true);
        QVERIFY(manager.popupActive());
        popupState.setProperty("opened", false);
        QVERIFY(!manager.popupActive());
        current.forceActiveFocus();

        QVERIFY(manager.moveFocus(0, 1));
        QCOMPARE(window.activeFocusItem(), &down);
        down.forceActiveFocus();
        QVERIFY(manager.moveFocus(1, 0));
        QCOMPARE(window.activeFocusItem(), &right);
    }

    void treatsOverlappingRowsAsTheSameNavigationColumn()
    {
        ControllerManager manager(nullptr, false);
        QQuickWindow window;
        window.setGeometry(0, 0, 900, 500);

        QQuickItem refresh(window.contentItem());
        refresh.setPosition({820, 20});
        refresh.setSize({50, 50});
        refresh.setActiveFocusOnTab(true);

        QQuickItem fullWidthMovie(window.contentItem());
        fullWidthMovie.setPosition({220, 120});
        fullWidthMovie.setSize({650, 120});
        fullWidthMovie.setActiveFocusOnTab(true);

        QQuickItem sidebarItem(window.contentItem());
        sidebarItem.setPosition({20, 100});
        sidebarItem.setSize({170, 50});
        sidebarItem.setActiveFocusOnTab(true);

        window.show();
        window.requestActivate();
        QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
        fullWidthMovie.forceActiveFocus();

        QVERIFY(manager.moveFocus(0, -1));
        QCOMPARE(window.activeFocusItem(), &refresh);
        QVERIFY(manager.moveFocus(0, 1));
        QCOMPARE(window.activeFocusItem(), &fullWidthMovie);
    }
};

QTEST_MAIN(ControllerManagerTest)
#include "tst_controller_manager.moc"
