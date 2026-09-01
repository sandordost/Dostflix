pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property var player
    property bool controlsVisible: true
    property int controlsHideInterval: Theme.controlsTimeout
    signal browseRequested()
    signal fullscreenRequested()
    signal findSubtitlesRequested()

    function formatTime(seconds) {
        if (!isFinite(seconds) || seconds < 0)
            return "0:00"
        const total = Math.floor(seconds)
        const hours = Math.floor(total / 3600)
        const minutes = Math.floor((total % 3600) / 60)
        const remainder = total % 60
        return hours > 0
                ? hours + ":" + String(minutes).padStart(2, "0") + ":" + String(remainder).padStart(2, "0")
                : minutes + ":" + String(remainder).padStart(2, "0")
    }

    function revealControls() {
        controlsVisible = true
        hideControls.restart()
    }

    function togglePlayback() {
        player.togglePaused()
        revealControls()
    }

    Timer {
        id: hideControls
        interval: root.controlsHideInterval
        repeat: false
        running: root.visible && root.player.hasActivePlayback
        onTriggered: {
            if (!root.player.paused && !root.player.buffering
                    && !topHover.hovered && !bottomHover.hovered)
                root.controlsVisible = false
            else
                restart()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
        cursorShape: root.controlsVisible ? Qt.ArrowCursor : Qt.BlankCursor
        onPositionChanged: root.revealControls()
    }

    PathPickerDialog {
        id: subtitleFileDialog
        title: qsTr("Choose subtitle file")
        fileNameFilters: ["*.srt", "*.ass", "*.vtt"]
        onPathChosen: path => root.player.addSubtitleFile(path)
    }

    Menu {
        id: subtitleMenu
        onAboutToShow: root.revealControls()
        background: Rectangle { radius: Theme.radius; color: Theme.surface }

        AppMenuItem {
            text: qsTr("No subtitles")
            checkable: true
            checked: root.player.selectedSubtitleId === "no"
            onTriggered: root.player.selectSubtitle("no")
        }

        Repeater {
            model: root.player.subtitleTracks
            delegate: AppMenuItem {
                required property var modelData
                text: modelData.label
                checkable: true
                checked: modelData.selected
                onTriggered: root.player.selectSubtitle(modelData.id)
            }
        }

        MenuSeparator {}
        AppMenuItem {
            objectName: "localSubtitleButton"
            text: qsTr("Open local subtitle…")
            onTriggered: subtitleFileDialog.openAt("")
        }
        AppMenuItem {
            objectName: "findSubtitlesButton"
            text: qsTr("Find subtitles…")
            onTriggered: root.findSubtitlesRequested()
        }
    }

    Rectangle {
        objectName: "playerTopBar"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: Theme.px(78)
        color: Qt.rgba(0.03, 0.03, 0.035, 0.94)
        opacity: root.controlsVisible ? 1 : 0
        visible: opacity > 0

        Behavior on opacity {
            NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic }
        }

        HoverHandler { id: topHover; onHoveredChanged: if (hovered) root.revealControls() }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.px(18)
            anchors.rightMargin: Theme.px(18)
            spacing: Theme.px(12)

            AppToolButton {
                objectName: "browseButton"
                icon.name: "go-previous-symbolic"
                icon.width: Theme.iconSizeLarge
                icon.height: Theme.iconSizeLarge
                Accessible.name: qsTr("Back to browse")
                ToolTip.visible: hovered
                ToolTip.text: Accessible.name
                onClicked: root.browseRequested()
            }
            Label {
                Layout.fillWidth: true
                text: root.player.activeTitle
                color: Theme.textPrimary
                font.pixelSize: Theme.headingSize
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            AppToolButton {
                icon.name: "view-fullscreen-symbolic"
                icon.width: Theme.iconSizeLarge
                icon.height: Theme.iconSizeLarge
                Accessible.name: qsTr("Toggle fullscreen")
                ToolTip.visible: hovered
                ToolTip.text: Accessible.name
                onClicked: root.fullscreenRequested()
            }
            AppToolButton {
                objectName: "stopButton"
                icon.name: "media-playback-stop-symbolic"
                icon.width: Theme.iconSize
                icon.height: Theme.iconSize
                Accessible.name: qsTr("Stop playback")
                ToolTip.visible: hovered
                ToolTip.text: Accessible.name
                onClicked: {
                    root.player.stop()
                    root.browseRequested()
                }
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        width: Theme.px(58)
        height: Theme.px(58)
        running: root.player.buffering
        visible: running
    }

    AppToolButton {
        anchors.centerIn: parent
        width: Theme.px(74)
        height: Theme.px(74)
        round: true
        primary: true
        visible: root.controlsVisible && !root.player.buffering
        opacity: visible ? 0.96 : 0
        symbol: root.player.paused ? "\uf04b" : "\uf04c"
        icon.width: Theme.px(26)
        icon.height: Theme.px(26)
        Accessible.name: root.player.paused ? qsTr("Play") : qsTr("Pause")
        onClicked: root.togglePlayback()

        Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
    }

    Rectangle {
        objectName: "playerControls"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: controls.implicitHeight + Theme.px(32)
        color: Qt.rgba(0.03, 0.03, 0.035, 0.96)
        opacity: root.controlsVisible ? 1 : 0
        visible: opacity > 0

        Behavior on opacity {
            NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic }
        }

        HoverHandler { id: bottomHover; onHoveredChanged: if (hovered) root.revealControls() }

        ColumnLayout {
            id: controls
            anchors.fill: parent
            anchors.margins: Theme.px(16)
            spacing: Theme.px(10)

            Slider {
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, root.player.duration)
                value: root.player.position
                Accessible.name: qsTr("Playback position")
                onMoved: {
                    root.player.setPosition(value)
                    root.revealControls()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.px(10)

                AppToolButton {
                    icon.name: "media-seek-backward-symbolic"
                    icon.width: Theme.iconSizeLarge
                    icon.height: Theme.iconSizeLarge
                    Accessible.name: qsTr("Back 10 seconds")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    onClicked: { root.player.seek(-10); root.revealControls() }
                }
                AppToolButton {
                    objectName: "pauseButton"
                    icon.name: root.player.paused ? "media-playback-start-symbolic" : "media-playback-pause-symbolic"
                    icon.width: Theme.iconSizeLarge
                    icon.height: Theme.iconSizeLarge
                    Accessible.name: root.player.paused ? qsTr("Play") : qsTr("Pause")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    onClicked: root.togglePlayback()
                }
                AppToolButton {
                    icon.name: "media-seek-forward-symbolic"
                    icon.width: Theme.iconSizeLarge
                    icon.height: Theme.iconSizeLarge
                    Accessible.name: qsTr("Forward 10 seconds")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    onClicked: { root.player.seek(10); root.revealControls() }
                }
                Label {
                    text: root.formatTime(root.player.position) + " / " + root.formatTime(root.player.duration)
                    color: Theme.textSecondary
                    font.pixelSize: Theme.captionSize
                }

                Item { Layout.fillWidth: true }

                AppToolButton {
                    objectName: "subtitleButton"
                    text: "CC"
                    font.weight: Font.DemiBold
                    Accessible.name: qsTr("Subtitles")
                    ToolTip.visible: hovered
                    ToolTip.text: Accessible.name
                    onClicked: subtitleMenu.popup()
                }
                Label {
                    text: qsTr("Delay")
                    color: Theme.textSecondary
                    visible: root.width >= Theme.px(760)
                }
                AppSpinBox {
                    objectName: "subtitleDelayControl"
                    visible: root.width >= Theme.px(760)
                    from: -600
                    to: 600
                    stepSize: 5
                    value: Math.round(root.player.subtitleDelay * 10)
                    editable: true
                    textFromValue: function(value) { return (value / 10).toFixed(1) + " s" }
                    valueFromText: function(text) {
                        const parsed = Number.parseFloat(text)
                        return Number.isFinite(parsed) ? Math.round(parsed * 10) : 0
                    }
                    onValueModified: root.player.setSubtitleDelay(value / 10)
                }
                AppToolButton {
                    icon.name: "audio-volume-high-symbolic"
                    icon.width: Theme.iconSize
                    icon.height: Theme.iconSize
                    visible: root.width >= Theme.px(620)
                    Accessible.name: qsTr("Volume")
                }
                Slider {
                    Layout.preferredWidth: Math.min(Theme.px(150), Math.max(Theme.px(86), root.width * 0.12))
                    visible: root.width >= Theme.px(620)
                    from: 0
                    to: 100
                    value: root.player.volume
                    Accessible.name: qsTr("Volume")
                    onMoved: root.player.setVolume(value)
                }
            }

            Label {
                Layout.fillWidth: true
                visible: root.player.errorMessage.length > 0
                text: root.player.errorMessage
                color: Theme.danger
                wrapMode: Text.WordWrap
            }
        }
    }

    Shortcut { sequence: "Space"; onActivated: root.togglePlayback() }
    Shortcut { sequence: "Left"; onActivated: { root.player.seek(-10); root.revealControls() } }
    Shortcut { sequence: "Right"; onActivated: { root.player.seek(10); root.revealControls() } }
    Shortcut { sequence: "F"; onActivated: { root.fullscreenRequested(); root.revealControls() } }

    Connections {
        target: root.player
        function onPausedChanged() { root.revealControls() }
        function onBufferingChanged() { root.revealControls() }
    }

    onVisibleChanged: if (visible) revealControls()
    Component.onCompleted: revealControls()
}
