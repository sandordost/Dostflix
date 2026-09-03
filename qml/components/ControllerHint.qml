import QtQuick
import QtQuick.Controls
import Dostflix

Rectangle {
    id: root
    required property string buttonLabel
    property string description: ""
    implicitWidth: Math.max(Theme.px(28), hintLabel.implicitWidth + Theme.px(14))
    implicitHeight: Theme.px(28)
    radius: height / 2
    color: Theme.raised
    border.width: Theme.px(1)
    border.color: Theme.separator

    Label {
        id: hintLabel
        anchors.centerIn: parent
        text: root.buttonLabel
        color: Theme.textPrimary
        font.family: Theme.fontFamily
        font.pixelSize: Theme.captionSize
        font.weight: Font.Bold
    }

    HoverHandler { id: hover }
    ToolTip.visible: hover.hovered && root.description.length > 0
    ToolTip.text: root.description
}
