import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

ApplicationWindow {
    id: window
    required property var appController
    required property var movieModel
    required property var vpnManager
    required property var providerManager
    required property var prowlarrManager
    required property var torrentEngine
    width: 1280
    height: 760
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: qsTr("Dostflix")
    color: Theme.canvas
    property int pageIndex: 0
    property bool showingPlayer: false
    property string launchedStreamUrl: ""

    function openReadyStream() {
        const url = window.torrentEngine.streamUrl
        if (!window.torrentEngine.bufferReady || url.length === 0
                || url === window.launchedStreamUrl)
            return
        window.launchedStreamUrl = url
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
            if (!hasActivePlayback)
                window.showingPlayer = false
        }
    }

    Connections {
        target: window.torrentEngine
        function onStatisticsChanged() { window.openReadyStream() }
        function onStateChanged() { window.openReadyStream() }
    }

    Image {
        anchors.fill: parent
        source: "qrc:/qt/qml/Dostflix/assets/backgrounds/dust-background.jpg"
        fillMode: Image.PreserveAspectCrop
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        AppHeader {
            Layout.fillWidth: true
            vpnLabel: window.vpnManager.stateLabel
            vpnConnected: window.vpnManager.connected
            vpnBusy: window.vpnManager.busy
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            SidePanel {
                Layout.preferredWidth: 255
                Layout.fillHeight: true
                currentIndex: window.pageIndex
                searchEnabled: window.prowlarrManager.ready
                onPageRequested: index => window.pageIndex = index
                onSearchRequested: query => window.prowlarrManager.search(query)
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: Theme.radius
                color: Qt.rgba(0.071, 0.071, 0.078, Theme.panelOpacity)

                StackLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    currentIndex: window.pageIndex
                    DiscoverPage {
                        movieModel: window.movieModel
                        prowlarrManager: window.prowlarrManager
                        torrentEngine: window.torrentEngine
                    }
                    LibraryPage {}
                    DownloadsPage {}
                    SettingsPage {
                        vpnManager: window.vpnManager
                        providerManager: window.providerManager
                        prowlarrManager: window.prowlarrManager
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
        onFindSubtitlesRequested: subtitleSearchNotice.open()
    }

    Dialog {
        id: subtitleSearchNotice
        anchors.centerIn: parent
        title: qsTr("Find subtitles")
        modal: true
        standardButtons: Dialog.Ok
        Label {
            width: 420
            text: qsTr("OpenSubtitles search is the next integration step. Embedded and local subtitle files already work without an account.")
            wrapMode: Text.WordWrap
            color: Theme.textPrimary
        }
    }
}
