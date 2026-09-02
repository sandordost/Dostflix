import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Dostflix

ApplicationWindow {
    id: window
    required property var appController
    required property var controllerManager
    required property var movieModel
    required property var libraryManager
    required property var metadataManager
    required property var downloadManager
    required property var vpnManager
    required property var providerManager
    required property var prowlarrManager
    required property var torrentEngine
    required property var subtitleManager
    required property bool gamescopeSession
    width: gamescopeSession ? Screen.width : Math.min(1280, Screen.width)
    height: gamescopeSession ? Screen.height : Math.min(800, Screen.height)
    minimumWidth: gamescopeSession ? 0 : Math.min(780, Screen.width)
    minimumHeight: gamescopeSession ? 0 : Math.min(520, Screen.height)
    visibility: gamescopeSession ? Window.FullScreen : Window.Windowed
    font.family: Theme.fontFamily
    font.pixelSize: Theme.bodySize
    title: qsTr("Dostflix")
    color: Theme.canvas
    palette.window: Theme.panel
    palette.windowText: Theme.textPrimary
    palette.base: Theme.input
    palette.alternateBase: Theme.surface
    palette.text: Theme.textPrimary
    palette.button: Theme.button
    palette.buttonText: Theme.buttonText
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.textPrimary
    palette.placeholderText: Theme.textMuted
    property int pageIndex: 0
    property bool showingPlayer: false
    readonly property real uiScale: gamescopeSession
                                    ? Math.max(1.0, width / Theme.referenceWidth) : 1.0
    readonly property real referenceLayoutWidth: width / uiScale
    readonly property bool compactNavigation: referenceLayoutWidth < 980
    property string launchedStreamUrl: ""
    property string stoppedStreamUrl: ""
    property bool controllerWasConnected: controllerManager.connected

    Binding {
        target: Theme
        property: "scaleFactor"
        value: window.uiScale
    }

    Binding {
        target: Theme
        property: "controllerConnected"
        value: window.controllerManager.connected
    }

    function openReadyStream() {
        const url = window.torrentEngine.streamUrl
        if (!window.torrentEngine.bufferReady || url.length === 0
                || url === window.launchedStreamUrl
                || url === window.stoppedStreamUrl)
            return
        window.launchedStreamUrl = url
        window.libraryManager.clearPlaybackSession()
        window.subtitleManager.setMediaContext("", "")
        videoPlayer.play(url, window.torrentEngine.title)
        window.showingPlayer = true
    }

    function activateControllerFocus() {
        window.controllerManager.activateFocus()
    }

    function focusPageEntry(index) {
        let focused = false
        if (index === 0)
            focused = discoverPage.focusFirstControl()
        else if (index === 1)
            focused = libraryPage.focusFirstControl()
        else if (index === 2)
            focused = downloadsPage.focusFirstControl()
        else if (index === 3)
            focused = settingsPage.focusFirstControl()
        if (focused) return
        sidePanel.focusCurrentPage()
    }

    onClosing: videoPlayer.stop()

    MpvPlayer {
        id: videoPlayer
        objectName: "videoPlayer"
        anchors.fill: parent
        visible: true
        z: window.showingPlayer ? 10 : -1
        onActivePlaybackChanged: {
            if (!hasActivePlayback) {
                if (window.launchedStreamUrl.length > 0)
                    window.stoppedStreamUrl = window.launchedStreamUrl
                window.showingPlayer = false
                window.launchedStreamUrl = ""
            }
        }
    }

    Connections {
        target: window.torrentEngine
        function onStatisticsChanged() { window.openReadyStream() }
        function onStateChanged() { window.openReadyStream() }
    }

    Connections {
        target: window.libraryManager
        function onPlaybackRequested(fileUrl, title, startSeconds) {
            window.launchedStreamUrl = ""
            videoPlayer.play(fileUrl, title, startSeconds)
            window.showingPlayer = true
        }
    }

    Connections {
        target: window.downloadManager
        function onLocalPlaybackRequested(fileUrl, title) {
            window.libraryManager.clearPlaybackSession()
            window.subtitleManager.setMediaContext(fileUrl, "")
            window.stoppedStreamUrl = ""
            window.launchedStreamUrl = ""
            videoPlayer.play(fileUrl, title)
            window.showingPlayer = true
        }
        function onTorrentPlaybackRequested() {
            if (videoPlayer.hasActivePlayback
                    && window.torrentEngine.streamUrl === window.launchedStreamUrl) {
                window.showingPlayer = true
                return
            }
            window.stoppedStreamUrl = ""
            window.launchedStreamUrl = ""
            window.openReadyStream()
        }
    }

    Connections {
        target: window.controllerManager

        function onConnectionChanged() {
            if (window.controllerWasConnected && !window.controllerManager.connected
                    && videoPlayer.hasActivePlayback && !videoPlayer.paused)
                videoPlayer.togglePaused()
            window.controllerWasConnected = window.controllerManager.connected
            if (window.controllerManager.connected && !window.showingPlayer)
                Qt.callLater(sidePanel.focusCurrentPage)
        }

        function onNavigationRequested(horizontal, vertical) {
            if (Theme.controllerKeyboardOpen) {
                Theme.activeControllerKeyboard.handleControllerNavigation(
                            horizontal, vertical)
                return
            }
            if (subtitleSearch.opened
                    && subtitleSearch.handleControllerNavigation(
                        horizontal, vertical))
                return
            if (sidePanel.controllerSearchActive)
                return
            if (window.showingPlayer) {
                playerPage.revealControls()
                if (playerPage.handleControllerNavigation(horizontal, vertical))
                    return
                window.controllerManager.moveFocus(horizontal, vertical)
            } else if (window.pageIndex === 0
                       && discoverPage.handleControllerNavigation(
                           horizontal, vertical)) {
                return
            } else if (window.pageIndex === 1
                       && libraryPage.handleControllerNavigation(
                           horizontal, vertical)) {
                return
            } else if (window.pageIndex === 3
                       && settingsPage.handleControllerNavigation(
                           horizontal, vertical)) {
                return
            } else {
                window.controllerManager.moveFocus(horizontal, vertical)
            }
        }

        function onConfirmRequested() {
            if (window.showingPlayer && playerPage.finishVolumeAdjustment())
                return
            if (!window.showingPlayer && window.pageIndex === 3
                    && settingsPage.activateControllerPopup())
                return
            window.activateControllerFocus()
        }

        function onBackRequested() {
            if (Theme.controllerKeyboardOpen) {
                Theme.activeControllerKeyboard.close()
                return
            }
            if (sidePanel.closeControllerSearch())
                return
            if (window.showingPlayer && playerPage.finishVolumeAdjustment())
                return
            if (window.showingPlayer && playerPage.subtitleMenuOpened) {
                playerPage.closeSubtitleMenu()
                return
            }
            if (window.controllerManager.popupActive()) {
                window.controllerManager.sendKey(Qt.Key_Escape)
                return
            }
            if (!window.showingPlayer && videoPlayer.hasActivePlayback) {
                window.showingPlayer = true
                return
            }
            window.controllerManager.sendKey(Qt.Key_Escape)
        }

        function onPlayPauseRequested() {
            if (videoPlayer.hasActivePlayback) {
                videoPlayer.togglePaused()
                if (window.showingPlayer) playerPage.revealControls()
            }
        }

        function changePageOrSeek(delta) {
            if (window.controllerManager.popupActive()
                    && !playerPage.subtitleMenuOpened)
                return
            if (window.showingPlayer) {
                videoPlayer.seek(delta * 30)
                playerPage.revealControls()
            } else {
                window.pageIndex = Math.max(0, Math.min(3, window.pageIndex + delta))
                Qt.callLater(function() { window.focusPageEntry(window.pageIndex) })
            }
        }

        function onPreviousPageRequested() {
            if (Theme.controllerKeyboardOpen) {
                Theme.activeControllerKeyboard.toggleShift()
                return
            }
            changePageOrSeek(-1)
        }
        function onNextPageRequested() {
            if (Theme.controllerKeyboardOpen) {
                Theme.activeControllerKeyboard.acceptInput()
                return
            }
            changePageOrSeek(1)
        }

        function onFullscreenRequested() {
            if (Theme.controllerKeyboardOpen) {
                Theme.activeControllerKeyboard.togglePlacement()
                return
            }
            if (window.controllerManager.popupActive()
                    && !playerPage.subtitleMenuOpened)
                return
            if (window.showingPlayer)
                playerPage.fullscreenRequested()
            else
                sidePanel.openControllerSearch()
        }

        function onSubtitlesRequested() {
            if (Theme.controllerKeyboardOpen) {
                Theme.activeControllerKeyboard.deletePrevious()
                return
            }
            if (window.showingPlayer)
                playerPage.openSubtitleMenu()
        }
    }

    Image {
        anchors.fill: parent
        source: "qrc:/qt/qml/Dostflix/assets/backgrounds/dust-background.jpg"
        fillMode: Image.PreserveAspectCrop
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.01, 0.015, 0.03, 0.52)
    }

    Shortcut { sequence: "Ctrl+1"; onActivated: window.pageIndex = 0 }
    Shortcut { sequence: "Ctrl+2"; onActivated: window.pageIndex = 1 }
    Shortcut { sequence: "Ctrl+3"; onActivated: window.pageIndex = 2 }
    Shortcut { sequence: "Ctrl+4"; onActivated: window.pageIndex = 3 }
    Shortcut {
        sequence: "Escape"
        enabled: window.showingPlayer
        onActivated: window.showingPlayer = false
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.px(18)
        anchors.rightMargin: Theme.px(18)
        anchors.topMargin: Theme.px(8)
        anchors.bottomMargin: Theme.px(16)
        spacing: Theme.contentGap

        AppHeader {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.headerHeight
            vpnLabel: window.vpnManager.stateLabel
            vpnConnected: window.vpnManager.connected
            vpnBusy: window.vpnManager.busy
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            enabled: !window.showingPlayer
            spacing: Theme.contentGap

            SidePanel {
                id: sidePanel
                Layout.preferredWidth: window.compactNavigation
                                       ? Theme.sidebarCompactWidth : Theme.sidebarWidth
                Layout.fillHeight: true
                compact: window.compactNavigation
                controllerManager: window.controllerManager
                currentIndex: window.pageIndex
                searchEnabled: window.prowlarrManager.ready
                onPageRequested: index => window.pageIndex = index
                onSearchRequested: query => window.prowlarrManager.search(query)
                onControllerSearchDismissed: searched => {
                    if (searched && discoverPage.focusFirstResult())
                        return
                    sidePanel.restoreControllerFocus()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: Theme.radiusLarge
                color: Qt.rgba(0.071, 0.071, 0.078, Theme.panelOpacity)

                StackLayout {
                    anchors.fill: parent
                    anchors.margins: window.referenceLayoutWidth < 900
                                     ? Theme.px(14) : Theme.pagePadding
                    currentIndex: window.pageIndex
                    DiscoverPage {
                        id: discoverPage
                        movieModel: window.movieModel
                        prowlarrManager: window.prowlarrManager
                        torrentEngine: window.torrentEngine
                    }
                    LibraryPage {
                        id: libraryPage
                        libraryManager: window.libraryManager
                        metadataManager: window.metadataManager
                    }
                    DownloadsPage {
                        id: downloadsPage
                        downloadManager: window.downloadManager
                    }
                    SettingsPage {
                        id: settingsPage
                        vpnManager: window.vpnManager
                        providerManager: window.providerManager
                        prowlarrManager: window.prowlarrManager
                        subtitleManager: window.subtitleManager
                        libraryManager: window.libraryManager
                        controllerManager: window.controllerManager
                    }
                }
            }
        }
    }

    NowWatchingCard {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.px(24)
        z: 3
        visible: videoPlayer.hasActivePlayback && !window.showingPlayer
        controller: videoPlayer
        controllerManager: window.controllerManager
        onReturnRequested: window.showingPlayer = true
    }

    PlayerPage {
        id: playerPage
        anchors.fill: parent
        visible: window.showingPlayer
        z: 11
        player: videoPlayer
        onBrowseRequested: window.showingPlayer = false
        onFullscreenRequested: {
            window.visibility = window.visibility === Window.FullScreen
                              ? Window.Windowed : Window.FullScreen
        }
        onFindSubtitlesRequested: {
            subtitleSearch.query = videoPlayer.activeTitle
            subtitleSearch.open()
        }
    }

    onShowingPlayerChanged: {
        if (showingPlayer)
            Qt.callLater(playerPage.focusDefaultControl)
    }

    SubtitleSearchDialog {
        id: subtitleSearch
        manager: window.subtitleManager
        onSettingsRequested: {
            close()
            window.showingPlayer = false
            window.pageIndex = 3
        }
    }
}
