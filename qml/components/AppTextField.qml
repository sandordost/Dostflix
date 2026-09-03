import QtQuick
import QtQuick.Controls
import Dostflix

TextField {
    id: root
    readonly property alias controllerKeyboardOpened: controllerKeyboard.opened
    implicitHeight: Theme.px(44)
    leftPadding: Theme.px(14)
    rightPadding: Theme.px(14)
    color: Theme.textPrimary
    placeholderTextColor: Theme.textMuted
    selectionColor: Theme.accent
    selectedTextColor: Theme.textPrimary
    font.family: Theme.fontFamily
    font.pixelSize: Theme.bodySize

    function controllerActivate() {
        root.forceActiveFocus(Qt.TabFocusReason)
        if (Theme.controllerConnected)
            controllerKeyboard.openForTarget()
    }

    function controllerAccept() {
        root.accepted()
    }

    ControllerKeyboard {
        id: controllerKeyboard
        targetField: root
    }

    background: Rectangle {
        radius: Theme.radius
        color: root.activeFocus ? Theme.raisedHover : Theme.input
        border.width: 0
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
    }
}
