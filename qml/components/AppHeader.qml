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
        width: Theme.px(52)
        height: Theme.px(52)
        source: "qrc:/qt/qml/Dostflix/assets/icons/dostflix.svg"
        sourceSize: Qt.size(Theme.px(104), Theme.px(104))
        Accessible.name: qsTr("Dostflix logo")
    }

    Label {
        id: brandTitle
        anchors.left: logo.right
        anchors.leftMargin: Theme.px(14)
        anchors.verticalCenter: logo.verticalCenter
        text: "Dostflix"
        color: Theme.textPrimary
        font.family: Theme.fontFamily
        font.pixelSize: Theme.px(31)
        font.weight: Font.DemiBold
    }

    Rectangle {
        id: vpnStatus
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: statusRow.implicitWidth + Theme.px(24)
        height: Theme.px(38)
        radius: Theme.px(19)
        color: Qt.rgba(0.08, 0.08, 0.09, 0.82)

        RowLayout {
            id: statusRow
            anchors.centerIn: parent
            spacing: Theme.px(8)

            BusyIndicator {
                Layout.preferredWidth: Theme.px(16)
                Layout.preferredHeight: Theme.px(16)
                running: root.vpnBusy
                visible: running
            }
            Rectangle {
                Layout.preferredWidth: Theme.px(8)
                Layout.preferredHeight: Theme.px(8)
                radius: Theme.px(4)
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
