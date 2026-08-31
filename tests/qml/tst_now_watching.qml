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

    ApplicationWindow {
        width: 500
        height: 200
        visible: true

        NowWatchingCard {
            id: card
            controller: fakeController
        }
    }

    function test_visibility_tracks_session() {
        compare(card.visible, false)
        fakeController.hasActivePlayback = true
        compare(card.controller.hasActivePlayback, true)
        tryCompare(card, "visible", true)
    }
}
