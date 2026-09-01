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
    required property string sourceLabel
    signal selected()
    width: Theme.posterWidth
    height: width / Theme.posterAspectRatio + 62
    scale: cardMouse.containsMouse ? 1.025 : 1
    z: cardMouse.containsMouse ? 2 : 0
    Accessible.role: Accessible.Button
    Accessible.name: title
    Accessible.onPressAction: selected()

    Behavior on scale {
        NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic }
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.surface

        Rectangle {
            id: posterFrame
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: root.width / Theme.posterAspectRatio
            radius: Theme.radius
            color: Theme.raised
            clip: true

            Image {
                id: poster
                anchors.fill: parent
                source: root.posterUrl.toString().length > 0
                        ? root.posterUrl
                        : "qrc:/qt/qml/Dostflix/assets/images/poster-placeholder.svg"
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                sourceSize: Qt.size(root.width * 2, posterFrame.height * 2)
            }

            Rectangle {
                anchors.fill: parent
                color: Qt.rgba(0, 0, 0, cardMouse.containsMouse ? 0.20 : 0)
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 9
                visible: root.quality.length > 0
                implicitWidth: qualityLabel.implicitWidth + 14
                height: 26
                radius: 13
                color: Qt.rgba(0.02, 0.02, 0.025, 0.88)
                Label {
                    id: qualityLabel
                    anchors.centerIn: parent
                    text: root.quality
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.captionSize
                    font.weight: Font.DemiBold
                }
            }

            Rectangle {
                anchors.centerIn: parent
                width: 52
                height: 52
                radius: 26
                color: Theme.button
                opacity: cardMouse.containsMouse ? 1 : 0
                scale: cardMouse.containsMouse ? 1 : 0.88
                Label {
                    anchors.centerIn: parent
                    anchors.horizontalCenterOffset: 2
                    text: "▶"
                    color: Theme.buttonText
                    font.pixelSize: 22
                }
                Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
                Behavior on scale { NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic } }
            }
        }

        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: posterFrame.bottom
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            anchors.topMargin: 9
            text: root.title
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.bodySize
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            anchors.bottomMargin: 8
            text: (root.year > 0 ? root.year : qsTr("Unknown year"))
                  + (root.seederCount > 0 ? qsTr(" · %1 seeds").arg(root.seederCount) : "")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.captionSize
            elide: Text.ElideRight
        }
    }

    MouseArea {
        id: cardMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.selected()
    }

    ToolTip.visible: cardMouse.containsMouse && root.sourceLabel.length > 0
    ToolTip.text: root.sourceLabel
}
