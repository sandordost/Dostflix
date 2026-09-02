import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "NowWatchingVisibility"
    when: windowShown

    QtObject {
        id: fakeController
        property bool hasActivePlayback: false
        property string activeTitle: ""
        property int watchedSeconds: 0
    }

    QtObject {
        id: fakeControllerManager
        property bool connected: true
        property string backButtonLabel: "B"
    }

    ApplicationWindow {
        width: 500
        height: 200
        visible: true

        NowWatchingCard {
            id: card
            controller: fakeController
            controllerManager: fakeControllerManager
        }
    }
    SignalSpy {
        id: returnSpy
        target: card
        signalName: "returnRequested"
    }

    function init() {
        fakeController.hasActivePlayback = false
        returnSpy.clear()
    }

    function test_visibility_tracks_session() {
        compare(card.visible, false)
        fakeController.hasActivePlayback = true
        compare(card.controller.hasActivePlayback, true)
        tryCompare(card, "visible", true)
    }

    function test_controller_activation_returns_to_movie() {
        fakeController.hasActivePlayback = true
        tryCompare(card, "visible", true)
        card.controllerActivate()
        compare(returnSpy.count, 1)
    }
}
