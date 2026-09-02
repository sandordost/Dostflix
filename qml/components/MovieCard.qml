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
    property bool transferActive: false
    property bool transferLoading: false
    property string transferStatusText: ""
    property bool transferHasError: false
    signal selected()
    activeFocusOnTab: true
    width: Theme.posterWidth
    height: width / Theme.posterAspectRatio + Theme.px(62)
    scale: cardMouse.containsMouse || activeFocus ? 1.025 : 1
    z: cardMouse.containsMouse || activeFocus ? 2 : 0
    Accessible.role: Accessible.Button
    Accessible.name: title
    Accessible.onPressAction: selected()

    function controllerActivate() {
        if (!transferLoading)
            selected()
    }

    Behavior on scale {
        NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic }
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.surface
        border.width: root.activeFocus ? Theme.px(2) : 0
        border.color: Theme.accent

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
                anchors.fill: parent
                visible: root.transferActive
                         && (root.transferLoading || root.transferHasError)
                color: Qt.rgba(0.02, 0.02, 0.025, 0.78)

                Column {
                    anchors.centerIn: parent
                    width: parent.width - Theme.px(20)
                    spacing: Theme.px(8)

                    BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: Theme.px(36)
                        height: Theme.px(36)
                        running: root.transferLoading
                        visible: running
                    }
                    AppIcon {
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: root.transferHasError && !root.transferLoading
                        glyph: "\uf071"
                        color: Theme.danger
                        font.pixelSize: Theme.iconSizeLarge
                    }
                    Label {
                        width: parent.width
                        text: root.transferStatusText
                        color: root.transferHasError ? Theme.danger : Theme.textPrimary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.captionSize
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }
                }
            }

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: Theme.px(9)
                visible: root.quality.length > 0
                implicitWidth: qualityLabel.implicitWidth + Theme.px(14)
                height: Theme.px(26)
                radius: Theme.px(13)
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
                width: Theme.px(52)
                height: Theme.px(52)
                radius: Theme.px(26)
                color: Theme.button
                opacity: cardMouse.containsMouse || root.activeFocus ? 1 : 0
                visible: !root.transferLoading && !root.transferHasError
                scale: cardMouse.containsMouse || root.activeFocus ? 1 : 0.88
                Label {
                    anchors.centerIn: parent
                    anchors.horizontalCenterOffset: Theme.px(2)
                    text: "▶"
                    color: Theme.buttonText
                    font.pixelSize: Theme.px(22)
                }
                Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
                Behavior on scale { NumberAnimation { duration: Theme.motionFast; easing.type: Easing.OutCubic } }
            }
        }

        Label {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: posterFrame.bottom
            anchors.leftMargin: Theme.px(10)
            anchors.rightMargin: Theme.px(10)
            anchors.topMargin: Theme.px(9)
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
            anchors.leftMargin: Theme.px(10)
            anchors.rightMargin: Theme.px(10)
            anchors.bottomMargin: Theme.px(8)
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
        onClicked: if (!root.transferLoading) root.selected()
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                || event.key === Qt.Key_Space) {
            root.controllerActivate()
            event.accepted = true
        }
    }

    ToolTip.visible: cardMouse.containsMouse && root.sourceLabel.length > 0
    ToolTip.text: root.sourceLabel
}
