import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Pane {
    id: root
    required property var controller
    signal returnRequested()
    visible: controller.hasActivePlayback
    implicitWidth: 360
    implicitHeight: 72
    padding: 10
    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Return to %1").arg(controller.activeTitle)
    Accessible.onPressAction: returnRequested()

    background: Rectangle {
        radius: Theme.radius
        color: hoverHandler.hovered ? Theme.raisedHover : Theme.raised
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
    }

    contentItem: RowLayout {
        spacing: 12

        Rectangle {
            Layout.preferredWidth: 46
            Layout.preferredHeight: 46
            radius: 23
            color: Theme.button
            Label {
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: 2
                text: "▶"
                color: Theme.buttonText
                font.pixelSize: 18
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Label {
                text: qsTr("Now playing")
                color: Theme.textSecondary
                font.pixelSize: Theme.captionSize
            }
            Label {
                Layout.fillWidth: true
                text: root.controller.activeTitle
                color: Theme.textPrimary
                font.pixelSize: Theme.bodySize
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            Label {
                text: qsTr("%1 watched").arg(root.formatDuration(root.controller.watchedSeconds))
                color: Theme.textMuted
                font.pixelSize: Theme.captionSize
            }
        }

        Label {
            text: "↗"
            color: Theme.textSecondary
            font.pixelSize: Theme.iconSize
        }
    }

    function formatDuration(seconds) {
        const total = Math.max(0, Math.floor(seconds))
        return Math.floor(total / 60) + ":" + String(total % 60).padStart(2, "0")
    }

    HoverHandler { id: hoverHandler }
    TapHandler {
        acceptedButtons: Qt.LeftButton
        onTapped: root.returnRequested()
    }
}
