import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property string vpnLabel
    required property bool vpnConnected
    required property bool vpnBusy
    implicitHeight: Theme.headerHeight

    Image {
        id: logo
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: 52
        height: 52
        source: "qrc:/qt/qml/Dostflix/assets/icons/dostflix.svg"
        sourceSize: Qt.size(104, 104)
        Accessible.name: qsTr("Dostflix logo")
    }

    Label {
        anchors.left: logo.right
        anchors.leftMargin: 14
        anchors.verticalCenter: logo.verticalCenter
        text: "Dostflix"
        color: Theme.textPrimary
        font.family: Theme.fontFamily
        font.pixelSize: 31
        font.weight: Font.DemiBold
    }

    Rectangle {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: statusRow.implicitWidth + 24
        height: 38
        radius: 19
        color: Qt.rgba(0.08, 0.08, 0.09, 0.82)

        RowLayout {
            id: statusRow
            anchors.centerIn: parent
            spacing: 8

            BusyIndicator {
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                running: root.vpnBusy
                visible: running
            }
            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 8
                radius: 4
                visible: !root.vpnBusy
                color: root.vpnConnected ? Theme.safe : Theme.textSecondary
            }
            Label {
                text: root.vpnLabel
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.captionSize
            }
        }
    }
}
