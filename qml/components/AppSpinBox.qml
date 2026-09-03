import QtQuick
import QtQuick.Controls
import Dostflix

SpinBox {
    id: root
    implicitWidth: Theme.px(132)
    implicitHeight: Theme.px(42)
    leftPadding: Theme.px(38)
    rightPadding: Theme.px(38)
    font.family: Theme.fontFamily
    font.pixelSize: Theme.bodySize
    focusPolicy: Qt.StrongFocus

    function controllerAdjust(direction) {
        if (direction === 0)
            return
        const nextValue = Math.max(root.from, Math.min(root.to,
                root.value + (direction > 0 ? root.stepSize : -root.stepSize)))
        if (nextValue === root.value)
            return
        root.value = nextValue
        root.valueModified()
    }

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
        implicitWidth: Theme.px(36)
        implicitHeight: Theme.px(42)
        radius: Theme.radius
        color: root.up.pressed ? Theme.buttonPressed
                               : (root.up.hovered ? Theme.buttonHover : "transparent")
        AppIcon {
            anchors.centerIn: parent
            glyph: "\u002b"
            font.family: Theme.fontFamily
            font.styleName: "Regular"
            font.weight: Font.DemiBold
            font.pixelSize: Theme.px(18)
        }
    }

    down.indicator: Rectangle {
        x: 0
        height: root.height
        implicitWidth: Theme.px(36)
        implicitHeight: Theme.px(42)
        radius: Theme.radius
        color: root.down.pressed ? Theme.buttonPressed
                                 : (root.down.hovered ? Theme.buttonHover : "transparent")
        AppIcon {
            anchors.centerIn: parent
            glyph: "\u2212"
            font.family: Theme.fontFamily
            font.styleName: "Regular"
            font.weight: Font.DemiBold
            font.pixelSize: Theme.px(18)
        }
    }

    background: Rectangle {
        radius: Theme.radius
        color: root.activeFocus ? Theme.raisedHover : Theme.input
        border.width: 0
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
    }
}
