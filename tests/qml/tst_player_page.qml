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
        property var subtitleTracks: [
            { id: "2", label: "English", language: "eng", selected: true, external: false }
        ]
        property string selectedSubtitleId: "2"
        property real subtitleDelay: 0
        property int pauseCalls: 0
        property int subtitleCalls: 0
        function stop() {}
        function seek(offset) {}
        function setPosition(seconds) {}
        function setVolume(value) {}
        function togglePaused() { pauseCalls += 1 }
        function selectSubtitle(id) { selectedSubtitleId = id; subtitleCalls += 1 }
        function addSubtitleFile(url) {}
        function setSubtitleDelay(seconds) { subtitleDelay = seconds }
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

    function test_subtitle_controls_exist() {
        verify(findChild(page, "subtitleButton") !== null)
        verify(findChild(page, "subtitleDelayControl") !== null)
        verify(findChild(page, "localSubtitleButton") !== null)
        verify(findChild(page, "findSubtitlesButton") !== null)
    }

}
