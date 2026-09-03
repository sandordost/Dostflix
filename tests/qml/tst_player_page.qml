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
        property bool muted: false
        property string errorMessage: ""
        property var subtitleTracks: [
            { id: "2", label: "English", language: "eng", selected: true, external: false }
        ]
        property string selectedSubtitleId: "2"
        property real subtitleDelay: 0
        property bool fillScreen: false
        property int pauseCalls: 0
        property int seekCalls: 0
        property int subtitleCalls: 0
        property int stopCalls: 0
        property int volumeCalls: 0
        property int muteCalls: 0
        function stop() { stopCalls += 1 }
        function seek(offset) { seekCalls += 1 }
        function setPosition(seconds) {}
        function setVolume(value) { volume = value; volumeCalls += 1 }
        function toggleMuted() { muted = !muted; muteCalls += 1 }
        function togglePaused() { pauseCalls += 1 }
        function selectSubtitle(id) { selectedSubtitleId = id; subtitleCalls += 1 }
        function addSubtitleFile(url) {}
        function setSubtitleDelay(seconds) { subtitleDelay = seconds }
        function toggleFillScreen() { fillScreen = !fillScreen }
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
        fakePlayer.muted = false
        fakePlayer.muteCalls = 0
        fakePlayer.subtitleDelay = 0
        fakePlayer.pauseCalls = 0
        fakePlayer.seekCalls = 0
        fakePlayer.subtitleCalls = 0
        fakePlayer.stopCalls = 0
        fakePlayer.fillScreen = false
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

    function test_fill_screen_toggle_is_controller_accessible() {
        const button = findChild(page, "fillScreenButton")
        verify(button !== null)
        button.forceActiveFocus(Qt.TabFocusReason)
        button.controllerActivate()
        compare(fakePlayer.fillScreen, true)
        compare(button.Accessible.name, "Fit video")
    }

    function test_center_pause_control_is_controller_default() {
        const button = findChild(page, "centerPauseButton")
        verify(button !== null)
        page.focusDefaultControl()
        tryCompare(button, "activeFocus", true)
        tryCompare(button, "scale", 1.12)
        button.controllerActivate()
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

    function test_delay_buttons_mute_and_volume_controller_adjustment() {
        const delay = findChild(page, "subtitleDelayControl")
        const decrease = findChild(page, "subtitleDelayDownButton")
        const increase = findChild(page, "subtitleDelayUpButton")
        const display = findChild(page, "subtitleDelayValue")
        const volume = findChild(page, "volumeButton")
        const volumeSlider = findChild(page, "volumeSlider")
        verify(delay !== null)
        verify(decrease !== null)
        verify(increase !== null)
        verify(display !== null)
        verify(volume !== null)
        verify(volumeSlider !== null)

        compare(delay.activeFocusOnTab, false)
        compare(display.activeFocusOnTab, false)
        increase.forceActiveFocus(Qt.TabFocusReason)
        increase.controllerActivate()
        compare(fakePlayer.subtitleDelay, 0.5)
        decrease.forceActiveFocus(Qt.TabFocusReason)
        decrease.controllerActivate()
        compare(fakePlayer.subtitleDelay, 0)

        volume.controllerActivate()
        compare(fakePlayer.muted, true)
        compare(fakePlayer.muteCalls, 1)
        compare(page.volumeAdjustmentActive, false)

        volumeSlider.controllerActivate()
        compare(page.volumeAdjustmentActive, true)
        verify(page.handleControllerNavigation(-1, 0))
        compare(fakePlayer.volume, 75)
        verify(fakePlayer.volumeCalls > 0)
        verify(page.finishVolumeAdjustment())
        compare(page.volumeAdjustmentActive, false)
        compare(volumeSlider.activeFocus, true)
    }

    function test_vertical_player_focus_visits_center_pause() {
        const browse = findChild(page, "browseButton")
        const center = findChild(page, "centerPauseButton")
        const timeline = findChild(page, "positionSlider")
        verify(browse !== null)
        verify(center !== null)
        verify(timeline !== null)

        browse.forceActiveFocus(Qt.TabFocusReason)
        verify(page.handleControllerNavigation(0, 1))
        tryCompare(center, "activeFocus", true)
        verify(page.handleControllerNavigation(0, 1))
        tryCompare(timeline, "activeFocus", true)
        verify(page.handleControllerNavigation(0, -1))
        tryCompare(center, "activeFocus", true)
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
