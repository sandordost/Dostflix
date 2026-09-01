import QtQuick
import QtQuick.Controls
import Dostflix

SpinBox {
    id: root
    implicitWidth: 132
    implicitHeight: 42
    leftPadding: 38
    rightPadding: 38
    font.family: Theme.fontFamily
    font.pixelSize: Theme.bodySize

    contentItem: TextInput {
        z: 2
        text: root.displayText
        color: Theme.textPrimary
        selectionColor: Theme.accent
        selectedTextColor: Theme.textPrimary
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        readOnly: !root.editable
        validator: root.validator
        inputMethodHints: root.inputMethodHints
        font: root.font
    }

    up.indicator: Rectangle {
        x: root.width - width
        height: root.height
        implicitWidth: 36
        implicitHeight: 42
        radius: Theme.radius
        color: root.up.pressed ? Theme.buttonPressed
                               : (root.up.hovered ? Theme.buttonHover : "transparent")
        AppIcon {
            anchors.centerIn: parent
            glyph: "\u002b"
            font.family: Theme.fontFamily
            font.styleName: "Regular"
            font.weight: Font.DemiBold
            font.pixelSize: 18
        }
    }

    down.indicator: Rectangle {
        x: 0
        height: root.height
        implicitWidth: 36
        implicitHeight: 42
        radius: Theme.radius
        color: root.down.pressed ? Theme.buttonPressed
                                 : (root.down.hovered ? Theme.buttonHover : "transparent")
        AppIcon {
            anchors.centerIn: parent
            glyph: "\u2212"
            font.family: Theme.fontFamily
            font.styleName: "Regular"
            font.weight: Font.DemiBold
            font.pixelSize: 18
        }
    }

    background: Rectangle {
        radius: Theme.radius
        color: root.activeFocus ? Theme.raisedHover : Theme.input
        border.width: 0
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
    }
}
