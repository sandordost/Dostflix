import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

ApplicationWindow {
    id: window
    required property var appController
    required property var movieModel
    required property var libraryManager
    required property var metadataManager
    required property var downloadManager
    required property var vpnManager
    required property var providerManager
    required property var prowlarrManager
    required property var torrentEngine
    required property var subtitleManager
    width: 1280
    height: 800
    minimumWidth: 780
    minimumHeight: 520
    visible: true
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
    readonly property bool compactNavigation: width < 980
    property string launchedStreamUrl: ""
    property string stoppedStreamUrl: ""

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
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        anchors.topMargin: 8
        anchors.bottomMargin: 16
        spacing: Theme.contentGap

        AppHeader {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.headerHeight
            vpnLabel: window.vpnManager.stateLabel
            vpnConnected: window.vpnManager.connected
            vpnBusy: window.vpnManager.busy
            prowlarrManager: window.prowlarrManager
            torrentEngine: window.torrentEngine
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.contentGap

            SidePanel {
                Layout.preferredWidth: window.compactNavigation
                                       ? Theme.sidebarCompactWidth : Theme.sidebarWidth
                Layout.fillHeight: true
                compact: window.compactNavigation
                currentIndex: window.pageIndex
                searchEnabled: window.prowlarrManager.ready
                onPageRequested: index => window.pageIndex = index
                onSearchRequested: query => window.prowlarrManager.search(query)
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: Theme.radiusLarge
                color: Qt.rgba(0.071, 0.071, 0.078, Theme.panelOpacity)

                StackLayout {
                    anchors.fill: parent
                    anchors.margins: window.width < 900 ? 14 : Theme.pagePadding
                    currentIndex: window.pageIndex
                    DiscoverPage {
                        movieModel: window.movieModel
                        prowlarrManager: window.prowlarrManager
                        torrentEngine: window.torrentEngine
                    }
                    LibraryPage {
                        libraryManager: window.libraryManager
                        metadataManager: window.metadataManager
                    }
                    DownloadsPage {
                        downloadManager: window.downloadManager
                    }
                    SettingsPage {
                        vpnManager: window.vpnManager
                        providerManager: window.providerManager
                        prowlarrManager: window.prowlarrManager
                        subtitleManager: window.subtitleManager
                        libraryManager: window.libraryManager
                    }
                }
            }
        }
    }

    NowWatchingCard {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24
        z: 3
        visible: videoPlayer.hasActivePlayback && !window.showingPlayer
        controller: videoPlayer
        onReturnRequested: window.showingPlayer = true
    }

    PlayerPage {
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
