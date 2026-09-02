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
        property int seekCalls: 0
        property int subtitleCalls: 0
        property int stopCalls: 0
        property int volumeCalls: 0
        function stop() { stopCalls += 1 }
        function seek(offset) { seekCalls += 1 }
        function setPosition(seconds) {}
        function setVolume(value) { volume = value; volumeCalls += 1 }
        function togglePaused() { pauseCalls += 1 }
        function selectSubtitle(id) { selectedSubtitleId = id; subtitleCalls += 1 }
        function addSubtitleFile(url) {}
        function setSubtitleDelay(seconds) { subtitleDelay = seconds }
    }

    ApplicationWindow {
        id: playerWindow
        width: 900
        height: 600
        visible: true
        PlayerPage {
            id: page
            anchors.fill: parent
            player: fakePlayer
            controlsHideInterval: 40
        }
    }
    SignalSpy {
        id: browseSpy
        target: page
        signalName: "browseRequested"
    }
    SignalSpy {
        id: findSubtitlesSpy
        target: page
        signalName: "findSubtitlesRequested"
    }

    function init() {
        playerWindow.requestActivate()
        tryCompare(playerWindow, "active", true)
        page.closeSubtitleMenu()
        tryCompare(page, "subtitleMenuOpened", false)
        page.controlsVisible = true
        fakePlayer.paused = false
        fakePlayer.buffering = false
        fakePlayer.volume = 80
        fakePlayer.volumeCalls = 0
        fakePlayer.subtitleDelay = 0
        findSubtitlesSpy.clear()
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

    function test_controller_can_open_subtitle_menu() {
        const menu = findChild(page, "subtitleMenu")
        const button = findChild(page, "subtitleButton")
        verify(menu !== null)
        verify(button !== null)
        mouseClick(button)
        tryCompare(menu, "opened", true)
        compare(page.subtitleMenuOpened, true)
        tryCompare(findChild(page, "noSubtitleButton"), "activeFocus", true)
        const startingIndex = menu.currentIndex
        page.navigateSubtitleMenu(1)
        verify(menu.currentIndex !== startingIndex)
        page.navigateSubtitleMenu(1)
        page.navigateSubtitleMenu(1)
        tryCompare(findChild(page, "findSubtitlesButton"), "activeFocus", true)
        playerWindow.activeFocusItem.controllerActivate()
        compare(findSubtitlesSpy.count, 1)
        tryCompare(page, "subtitleMenuOpened", false)
    }

    function test_keyboard_can_open_subtitle_menu() {
        const menu = findChild(page, "subtitleMenu")
        const button = findChild(page, "subtitleButton")
        button.forceActiveFocus(Qt.TabFocusReason)
        tryCompare(button, "activeFocus", true)
        keyClick(Qt.Key_Return)
        tryCompare(menu, "opened", true)
        page.closeSubtitleMenu()
    }

    function test_delay_and_volume_controller_adjustment() {
        const delay = findChild(page, "subtitleDelayControl")
        const volume = findChild(page, "volumeButton")
        verify(delay !== null)
        verify(volume !== null)

        delay.forceActiveFocus(Qt.TabFocusReason)
        verify(page.handleControllerNavigation(1, 0))
        compare(fakePlayer.subtitleDelay, 0.5)
        verify(page.handleControllerNavigation(-1, 0))
        compare(fakePlayer.subtitleDelay, 0)

        volume.controllerActivate()
        compare(page.volumeAdjustmentActive, true)
        verify(page.handleControllerNavigation(-1, 0))
        compare(fakePlayer.volume, 75)
        verify(fakePlayer.volumeCalls > 0)
        verify(page.finishVolumeAdjustment())
        compare(page.volumeAdjustmentActive, false)
        compare(volume.activeFocus, true)
    }

    function test_controls_auto_hide_and_reveal() {
        page.revealControls()
        mouseMove(page, page.width / 2, page.height / 2)
        tryCompare(page, "controlsVisible", false, 400)
        page.revealControls()
        mouseMove(page, page.width / 2, page.height / 2)
        compare(page.controlsVisible, true)
    }

    function test_small_pointer_jitter_does_not_block_auto_hide() {
        page.controlsHideInterval = 70
        page.revealControls()
        mouseMove(page, 400, 300)
        wait(15)
        mouseMove(page, 401, 300)
        wait(15)
        mouseMove(page, 400, 301)
        tryCompare(page, "controlsVisible", false, 300)
        page.controlsHideInterval = 40
    }

    function test_stop_control_stops_playback_and_browses() {
        const button = findChild(page, "stopButton")
        verify(button !== null)
        browseSpy.clear()
        page.revealControls()
        mouseMove(page, page.width / 2, page.height / 2)
        mouseClick(button)
        compare(fakePlayer.stopCalls, 1)
        compare(browseSpy.count, 1)
    }
}
