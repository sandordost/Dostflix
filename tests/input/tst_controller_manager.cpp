#include "input/ControllerManager.h"

#include <QSignalSpy>
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
};

QTEST_GUILESS_MAIN(ControllerManagerTest)
#include "tst_controller_manager.moc"

