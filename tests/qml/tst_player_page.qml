import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "PlayerPageControls"
    when: windowShown

    QtObject {
        id: fakePlayer
        property bool hasActivePlayback: true
        property string activeTitle: "Test movie"
        property real position: 65
        property real duration: 3600
        property bool paused: false
        property bool buffering: false
        property real volume: 80
        property string errorMessage: ""
        property int pauseCalls: 0
        function stop() {}
        function seek(offset) {}
        function setPosition(seconds) {}
        function setVolume(value) {}
        function togglePaused() { pauseCalls += 1 }
    }

    ApplicationWindow {
        width: 900
        height: 600
        visible: true
        PlayerPage {
            id: page
            anchors.fill: parent
            player: fakePlayer
        }
    }

    function test_formats_time() {
        compare(page.formatTime(0), "0:00")
        compare(page.formatTime(65), "1:05")
        compare(page.formatTime(3661), "1:01:01")
    }

    function test_pause_control() {
        const button = findChild(page, "pauseButton")
        verify(button !== null)
        mouseClick(button)
        compare(fakePlayer.pauseCalls, 1)
    }

}
