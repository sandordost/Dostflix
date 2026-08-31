import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Pane {
    id: root
    required property var controller
    signal returnRequested()
    visible: controller.hasActivePlayback
    width: 340

    background: Rectangle {
        radius: Theme.radius
        color: Qt.rgba(0.03, 0.03, 0.05, 0.96)
    }

    contentItem: RowLayout {
        spacing: 12
        Rectangle {
            Layout.preferredWidth: 52
            Layout.preferredHeight: 52
            radius: 5
            color: Theme.raised
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Label {
                text: qsTr("Now watching")
                color: Theme.textSecondary
                font.pixelSize: 10
            }
            Label {
                Layout.fillWidth: true
                text: root.controller.activeTitle
                color: Theme.textPrimary
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            Label {
                text: root.controller.watchedSeconds + qsTr(" seconds watched")
                color: Theme.textSecondary
                font.pixelSize: 10
            }
        }
        Button {
            text: qsTr("Return to movie")
            onClicked: root.returnRequested()
            Accessible.name: text
        }
    }
}
