pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property var player
    signal browseRequested()
    signal fullscreenRequested()

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

    Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 22
        spacing: 12
        Button {
            objectName: "browseButton"
            text: qsTr("← Browse")
            onClicked: root.browseRequested()
        }
        Label {
            Layout.fillWidth: true
            text: root.player.activeTitle
            color: "white"
            font.pixelSize: Theme.titleSize
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
        Button {
            text: qsTr("Fullscreen")
            onClicked: root.fullscreenRequested()
        }
        Button {
            text: qsTr("Stop")
            onClicked: {
                root.player.stop()
                root.browseRequested()
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        width: 52
        height: 52
        running: root.player.buffering
        visible: running
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: Qt.rgba(0.02, 0.02, 0.025, 0.94)
        height: controls.implicitHeight + 30

        ColumnLayout {
            id: controls
            anchors.fill: parent
            anchors.margins: 15
            spacing: 10

            Slider {
                Layout.fillWidth: true
                from: 0
                to: Math.max(1, root.player.duration)
                value: root.player.position
                onMoved: root.player.setPosition(value)
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Button {
                    text: qsTr("−10s")
                    onClicked: root.player.seek(-10)
                }
                Button {
                    objectName: "pauseButton"
                    text: root.player.paused ? qsTr("Play") : qsTr("Pause")
                    onClicked: root.player.togglePaused()
                }
                Button {
                    text: qsTr("+10s")
                    onClicked: root.player.seek(10)
                }
                Label {
                    text: root.formatTime(root.player.position) + " / " + root.formatTime(root.player.duration)
                    color: Theme.textPrimary
                }
                Item { Layout.fillWidth: true }
                Label {
                    text: qsTr("Volume")
                    color: Theme.textSecondary
                }
                Slider {
                    Layout.preferredWidth: 150
                    from: 0
                    to: 100
                    value: root.player.volume
                    onMoved: root.player.setVolume(value)
                }
            }

            Label {
                Layout.fillWidth: true
                visible: root.player.errorMessage.length > 0
                text: root.player.errorMessage
                color: "#ff9b9b"
                wrapMode: Text.WordWrap
            }
        }
    }
}
