pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Popup {
    id: root
    required property var targetField
    property bool shiftActive: false
    property bool capsLock: false
    readonly property bool uppercase: shiftActive || capsLock
    property var firstKeyButton: null
    property bool openedAbove: false
    property var keyboardRows: [
        [
            { value: "\u0060", shifted: "~" },
            { value: "1", shifted: "!" },
            { value: "2", shifted: "@" },
            { value: "3", shifted: "#" },
            { value: "4", shifted: "$" },
            { value: "5", shifted: "%" },
            { value: "6", shifted: "^" },
            { value: "7", shifted: "&" },
            { value: "8", shifted: "*" },
            { value: "9", shifted: "(" },
            { value: "0", shifted: ")" },
            { value: "-", shifted: "_" },
            { value: "=", shifted: "+" },
            { label: "X/□  Backspace", action: "backspace", units: 2.4 }
        ],
        [
            { label: "Tab", action: "tab", units: 1.45 },
            { value: "q" }, { value: "w" }, { value: "e" },
            { value: "r" }, { value: "t" }, { value: "y" },
            { value: "u" }, { value: "i" }, { value: "o" },
            { value: "p" }, { value: "[", shifted: "{" },
            { value: "]", shifted: "}" },
            { value: "\\", shifted: "|" }
        ],
        [
            { label: "Caps", action: "caps", units: 1.8 },
            { value: "a" }, { value: "s" }, { value: "d" },
            { value: "f" }, { value: "g" }, { value: "h" },
            { value: "j" }, { value: "k" }, { value: "l" },
            { value: ";", shifted: ":" },
            { value: "'", shifted: "\"" },
            { label: "R2  Done", action: "done", units: 2.1 }
        ],
        [
            { label: "L2  Shift", action: "shift", units: 2.1 },
            { value: "z" }, { value: "x" }, { value: "c" },
            { value: "v" }, { value: "b" }, { value: "n" },
            { value: "m" }, { value: ",", shifted: "<" },
            { value: ".", shifted: ">" },
            { value: "/", shifted: "?" },
            { label: "L2  Shift", action: "shift", units: 2.1 }
        ],
        [
            { label: "Y/△  Move", action: "move", units: 1.8 },
            { label: "←", action: "cursorLeft" },
            { label: "Space", action: "space", units: 7 },
            { label: "→", action: "cursorRight" },
            { label: "Clear", action: "clear", units: 1.5 },
            { label: "Done", action: "done", units: 1.5 }
        ]
    ]

    objectName: "controllerKeyboard"
    parent: targetField ? targetField.Overlay.overlay : null
    width: parent ? Math.min(Theme.px(1080), parent.width - Theme.px(32))
                  : Theme.px(1080)
    height: keyboardContent.implicitHeight + padding * 2
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? (openedAbove ? Theme.px(18)
                             : parent.height - height - Theme.px(18)) : 0
    padding: Theme.px(12)
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
        shiftActive = false
        capsLock = false
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
        targetField.insert(targetField.cursorPosition, value)
        if (shiftActive)
            shiftActive = false
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

    function moveCursor(offset) {
        targetField.cursorPosition = Math.max(0, Math.min(targetField.length,
                targetField.cursorPosition + offset))
    }

    function toggleShift() {
        shiftActive = !shiftActive
    }

    function togglePlacement() {
        openedAbove = !openedAbove
    }

    function displayText(key) {
        if (key.label)
            return key.label
        if (root.uppercase && key.shifted)
            return key.shifted
        const value = String(key.value)
        return root.uppercase ? value.toUpperCase() : value
    }

    function activateKey(key) {
        switch (key.action || "") {
        case "backspace": deletePrevious(); break
        case "tab": insertText("\t"); break
        case "caps": capsLock = !capsLock; break
        case "shift": toggleShift(); break
        case "done": acceptInput(); break
        case "space": insertText(" "); break
        case "cursorLeft": moveCursor(-1); break
        case "cursorRight": moveCursor(1); break
        case "clear": targetField.clear(); break
        case "move": togglePlacement(); break
        default: insertText(displayText(key)); break
        }
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
        color: Qt.rgba(0.045, 0.045, 0.052, 0.985)
        border.width: Theme.px(1)
        border.color: Theme.separator
    }

    contentItem: ColumnLayout {
        id: keyboardContent
        spacing: Theme.px(6)

        Repeater {
            id: rowRepeater
            model: root.keyboardRows
            delegate: RowLayout {
                id: keyboardRow
                required property int index
                required property var modelData
                Layout.fillWidth: true
                spacing: Theme.px(5)

                Repeater {
                    model: keyboardRow.modelData
                    delegate: AppButton {
                        id: keyButton
                        required property int index
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredWidth: Theme.px(52)
                                               * (modelData.units || 1)
                        Layout.preferredHeight: Theme.px(43)
                        text: root.displayText(modelData)
                        checkable: modelData.action === "caps"
                                   || modelData.action === "shift"
                        checked: modelData.action === "caps" ? root.capsLock
                                 : (modelData.action === "shift"
                                    ? root.shiftActive : false)
                        Accessible.name: modelData.action
                                ? text : qsTr("Type %1").arg(text)
                        onClicked: root.activateKey(modelData)
                        Component.onCompleted: {
                            if (keyboardRow.index === 0 && keyButton.index === 0)
                                root.firstKeyButton = keyButton
                        }
                    }
                }
            }
        }
    }
}
