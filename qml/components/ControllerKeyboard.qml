pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Popup {
    id: root
    required property var targetField
    property bool uppercase: false
    property var firstKeyButton: null
    property bool openedAbove: false
    property var keyboardRows: [
        ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"],
        ["q", "w", "e", "r", "t", "y", "u", "i", "o", "p"],
        ["a", "s", "d", "f", "g", "h", "j", "k", "l"],
        ["z", "x", "c", "v", "b", "n", "m"],
        ["-", "_", ".", ":", "/", "@", "+", "=", "?", "&", "%", "#"]
    ]

    objectName: "controllerKeyboard"
    parent: targetField ? targetField.Overlay.overlay : null
    width: parent ? Math.min(Theme.px(900), parent.width - Theme.px(32))
                  : Theme.px(900)
    height: keyboardContent.implicitHeight + padding * 2
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? (openedAbove ? Theme.px(18)
                             : parent.height - height - Theme.px(18)) : 0
    padding: Theme.px(14)
    modal: true
    dim: false
    focus: true
    closePolicy: Popup.CloseOnEscape
    popupType: Popup.Item

    function openForTarget() {
        if (!targetField || !targetField.enabled || targetField.readOnly)
            return false
        const overlay = targetField.Overlay.overlay
        openedAbove = overlay
                ? targetField.mapToItem(overlay, 0, targetField.height / 2).y
                        > overlay.height / 2
                : false
        uppercase = false
        open()
        return true
    }

    function replaceSelection() {
        if (targetField.selectionStart === targetField.selectionEnd)
            return
        const start = Math.min(targetField.selectionStart, targetField.selectionEnd)
        const end = Math.max(targetField.selectionStart, targetField.selectionEnd)
        targetField.remove(start, end)
        targetField.cursorPosition = start
    }

    function insertText(value) {
        replaceSelection()
        targetField.insert(targetField.cursorPosition,
                           uppercase ? value.toUpperCase() : value)
    }

    function deletePrevious() {
        if (targetField.selectionStart !== targetField.selectionEnd) {
            replaceSelection()
            return
        }
        if (targetField.cursorPosition <= 0)
            return
        const position = targetField.cursorPosition
        targetField.remove(position - 1, position)
    }

    function focusFirstKey() {
        if (firstKeyButton)
            firstKeyButton.forceActiveFocus(Qt.PopupFocusReason)
    }

    function navigationRows() {
        const rows = []
        for (let rowIndex = 0; rowIndex < rowRepeater.count; ++rowIndex) {
            const rowItem = rowRepeater.itemAt(rowIndex)
            const buttons = []
            if (rowItem) {
                for (let childIndex = 0; childIndex < rowItem.children.length;
                     ++childIndex) {
                    const child = rowItem.children[childIndex]
                    if (child && typeof child.controllerActivate === "function")
                        buttons.push(child)
                }
            }
            if (buttons.length > 0)
                rows.push(buttons)
        }
        rows.push([shiftButton, backspaceButton, spaceButton,
                   clearButton, doneButton])
        return rows
    }

    function activePosition(rows) {
        const focused = targetField && targetField.Window.window
                ? targetField.Window.window.activeFocusItem : null
        for (let rowIndex = 0; rowIndex < rows.length; ++rowIndex) {
            const column = rows[rowIndex].indexOf(focused)
            if (column >= 0)
                return { row: rowIndex, column: column }
        }
        return { row: 0, column: 0 }
    }

    function handleControllerNavigation(horizontal, vertical) {
        const rows = navigationRows()
        if (rows.length === 0)
            return true
        const position = activePosition(rows)
        let row = position.row
        let column = position.column
        if (vertical !== 0) {
            row = Math.max(0, Math.min(rows.length - 1,
                    row + (vertical > 0 ? 1 : -1)))
            column = Math.min(column, rows[row].length - 1)
        } else if (horizontal !== 0) {
            column = Math.max(0, Math.min(rows[row].length - 1,
                    column + (horizontal > 0 ? 1 : -1)))
        }
        rows[row][column].forceActiveFocus(Qt.TabFocusReason)
        return true
    }

    function acceptInput() {
        close()
    }

    onOpened: {
        Theme.activeControllerKeyboard = root
        Qt.callLater(focusFirstKey)
    }
    onClosed: {
        if (Theme.activeControllerKeyboard === root)
            Theme.activeControllerKeyboard = null
        if (targetField && targetField.visible && targetField.enabled)
            targetField.forceActiveFocus(Qt.PopupFocusReason)
    }
    Component.onDestruction: {
        if (Theme.activeControllerKeyboard === root)
            Theme.activeControllerKeyboard = null
    }

    background: Rectangle {
        radius: Theme.radiusLarge
        color: Qt.rgba(0.055, 0.055, 0.065, 0.98)
        border.width: Theme.px(1)
        border.color: Theme.separator
    }

    contentItem: ColumnLayout {
        id: keyboardContent
        spacing: Theme.px(7)

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: qsTr("Controller keyboard")
                color: Theme.textSecondary
                font.pixelSize: Theme.captionSize
            }
            Label {
                text: root.openedAbove ? qsTr("Shown above input")
                                       : qsTr("Shown below input")
                color: Theme.textMuted
                font.pixelSize: Theme.captionSize
            }
        }

        Repeater {
            id: rowRepeater
            model: root.keyboardRows
            delegate: RowLayout {
                id: keyboardRow
                required property int index
                required property var modelData
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                spacing: Theme.px(6)

                Repeater {
                    id: keyRepeater
                    model: keyboardRow.modelData
                    delegate: AppButton {
                        id: keyButton
                        required property int index
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: Theme.px(42)
                        text: root.uppercase ? String(modelData).toUpperCase()
                                             : String(modelData)
                        Accessible.name: qsTr("Type %1").arg(text)
                        onClicked: root.insertText(String(modelData))
                        Component.onCompleted: {
                            if (keyboardRow.index === 0 && keyButton.index === 0)
                                root.firstKeyButton = keyButton
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.px(7)

            AppButton {
                id: shiftButton
                text: root.uppercase ? qsTr("Lowercase") : qsTr("Shift")
                symbol: "\uf062"
                checkable: true
                checked: root.uppercase
                onClicked: root.uppercase = !root.uppercase
            }
            AppButton {
                id: backspaceButton
                text: qsTr("Backspace")
                symbol: "\uf55a"
                onClicked: root.deletePrevious()
            }
            AppButton {
                id: spaceButton
                Layout.fillWidth: true
                text: qsTr("Space")
                onClicked: root.insertText(" ")
            }
            AppButton {
                id: clearButton
                text: qsTr("Clear")
                onClicked: root.targetField.clear()
            }
            AppButton {
                id: doneButton
                text: qsTr("Done")
                primary: true
                onClicked: root.acceptInput()
            }
        }
    }
}
