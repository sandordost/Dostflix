import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "ControllerKeyboard"
    when: windowShown

    ApplicationWindow {
        id: keyboardWindow
        width: 1000
        height: 700
        visible: true

        AppTextField {
            id: topField
            objectName: "topField"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 80
            width: 500
        }

        AppTextField {
            id: bottomField
            objectName: "bottomField"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 80
            width: 500
        }

        AppTextField {
            id: readOnlyField
            visible: false
            readOnly: true
        }
    }

    function init() {
        keyboardWindow.requestActivate()
        tryCompare(keyboardWindow, "active", true)
        Theme.controllerConnected = true
        topField.text = ""
        bottomField.text = ""
    }

    function cleanup() {
        Theme.controllerConnected = false
    }

    function keyboardFor(field) {
        const keyboard = findChild(field, "controllerKeyboard")
        verify(keyboard !== null)
        return keyboard
    }

    function test_controller_activation_opens_and_types() {
        topField.controllerActivate()
        const keyboard = keyboardFor(topField)
        tryCompare(keyboard, "opened", true)
        compare(Theme.activeControllerKeyboard, keyboard)
        compare(keyboard.openedAbove, false)
        keyboard.insertText("d")
        keyboard.insertText("o")
        compare(topField.text, "do")
        keyboard.deletePrevious()
        compare(topField.text, "d")
        keyboard.acceptInput()
        tryCompare(keyboard, "opened", false)
        compare(Theme.controllerKeyboardOpen, false)
    }

    function test_directional_navigation_stays_inside_keyboard() {
        topField.controllerActivate()
        const keyboard = keyboardFor(topField)
        tryCompare(keyboard, "opened", true)
        tryCompare(keyboard.firstKeyButton, "activeFocus", true)
        keyboard.handleControllerNavigation(1, 0)
        compare(keyboardWindow.activeFocusItem.text, "2")
        keyboard.handleControllerNavigation(0, 1)
        compare(keyboardWindow.activeFocusItem.text, "w")
        keyboardWindow.activeFocusItem.controllerActivate()
        compare(topField.text, "w")
        keyboard.close()
    }

    function test_lower_input_places_keyboard_above() {
        bottomField.controllerActivate()
        const keyboard = keyboardFor(bottomField)
        tryCompare(keyboard, "opened", true)
        compare(keyboard.openedAbove, true)
        verify(keyboard.y < bottomField.y)
        keyboard.close()
    }

    function test_read_only_field_does_not_open_keyboard() {
        readOnlyField.controllerActivate()
        compare(readOnlyField.controllerKeyboardOpened, false)
    }
}
