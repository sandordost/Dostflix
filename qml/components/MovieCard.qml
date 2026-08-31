import QtQuick
import QtQuick.Controls
import Dostflix

Item {
    id: root
    required property string title
    required property int year
    required property string quality
    required property int seederCount
    required property url posterUrl
    width: 170
    height: width / Theme.posterAspectRatio + 62

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Qt.rgba(0.18, 0.18, 0.19, 0.78)

        Image {
            id: poster
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 8
            height: width / Theme.posterAspectRatio
            source: root.posterUrl
            fillMode: Image.PreserveAspectCrop
            asynchronous: true

            Rectangle {
                anchors.fill: parent
                color: "#343438"
                visible: poster.status !== Image.Ready
            }
        }

        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: poster.bottom
            anchors.margins: 8
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: Theme.bodySize
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 8
            text: root.year + " · " + root.quality + " · " + root.seederCount + qsTr(" seeders")
            color: Theme.textSecondary
            font.pixelSize: 11
            elide: Text.ElideRight
        }
    }
}
