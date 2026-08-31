import QtQuick
import QtQuick.Controls
import Dostflix

Item {
    id: root
    required property string vpnLabel
    height: 72

    Image {
        id: logo
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: 44
        height: 44
        source: "qrc:/qt/qml/Dostflix/assets/icons/dostflix.svg"
        Accessible.name: qsTr("Dostflix logo")
    }

    Label {
        anchors.left: logo.right
        anchors.leftMargin: 12
        anchors.verticalCenter: logo.verticalCenter
        text: "Dostflix"
        color: Theme.textPrimary
        font.pixelSize: 30
        font.weight: Font.Bold
    }

    Row {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Rectangle {
            width: 8
            height: 8
            radius: 4
            color: Theme.safe
        }
        Label {
            text: root.vpnLabel
            color: Theme.textSecondary
            font.pixelSize: 12
        }
    }
}
