pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Dostflix

ComboBox {
    id: root
    property int controllerHighlightedIndex: -1
    readonly property bool controllerPopupActive: popup.opened
    implicitHeight: Theme.px(44)
    leftPadding: Theme.px(14)
    rightPadding: Theme.px(42)
    font.family: Theme.fontFamily
    font.pixelSize: Theme.bodySize
    palette.text: Theme.textPrimary
    palette.buttonText: Theme.textPrimary
    palette.highlight: Theme.accentSoft
    palette.highlightedText: Theme.textPrimary
    focusPolicy: Qt.StrongFocus

    function controllerActivate() {
        if (root.count <= 0)
            return
        if (root.popup.opened) {
            const index = root.controllerHighlightedIndex >= 0
                    ? root.controllerHighlightedIndex
                    : Math.max(0, root.currentIndex)
            root.currentIndex = index
            root.activated(index)
            root.popup.close()
        } else {
            root.controllerHighlightedIndex = root.currentIndex >= 0
                    ? root.currentIndex : 0
            root.popup.open()
        }
    }

    function controllerNavigate(direction) {
        if (!root.popup.opened)
            return false
        if (root.count <= 0) {
            root.popup.close()
            return true
        }
        const previous = root.controllerHighlightedIndex >= 0
                ? root.controllerHighlightedIndex : Math.max(0, root.currentIndex)
        root.controllerHighlightedIndex = Math.max(0, Math.min(root.count - 1,
                previous + (direction > 0 ? 1 : -1)))
        popupList.currentIndex = root.controllerHighlightedIndex
        popupList.positionViewAtIndex(root.controllerHighlightedIndex,
                                      ListView.Contain)
        return true
    }

    contentItem: Text {
        text: root.displayText
        color: root.enabled ? Theme.textPrimary : Theme.textMuted
        font: root.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: AppIcon {
        x: root.width - width - Theme.px(14)
        y: (root.height - height) / 2
        glyph: "\uf078"
        color: Theme.textSecondary
        font.pixelSize: Theme.px(13)
    }

    background: Rectangle {
        radius: Theme.radius
        color: root.down || root.activeFocus ? Theme.raisedHover : Theme.input
        border.width: 0
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
    }

    delegate: ItemDelegate {
        id: optionDelegate
        required property int index
        width: root.width
        height: Theme.px(40)
        highlighted: root.popup.opened && root.controllerHighlightedIndex >= 0
                     ? root.controllerHighlightedIndex === index
                     : root.highlightedIndex === index
        contentItem: Text {
            text: root.textAt(optionDelegate.index)
            color: Theme.textPrimary
            font: root.font
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: Theme.radiusSmall
            color: optionDelegate.highlighted ? Theme.accentSoft
                                              : (optionDelegate.hovered
                                                 ? Theme.raisedHover : "transparent")
        }
    }

    popup: Popup {
        y: root.height + Theme.px(6)
        width: root.width
        implicitHeight: Math.min(contentItem.implicitHeight + Theme.px(12), Theme.px(320))
        padding: Theme.px(6)
        onOpened: {
            if (root.controllerHighlightedIndex < 0 && root.count > 0)
                root.controllerHighlightedIndex = Math.max(0, root.currentIndex)
            contentItem.currentIndex = root.controllerHighlightedIndex
            contentItem.forceActiveFocus(Qt.PopupFocusReason)
        }
        onClosed: {
            root.controllerHighlightedIndex = -1
            root.forceActiveFocus(Qt.PopupFocusReason)
        }
        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
        }
        contentItem: ListView {
            id: popupList
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.controllerHighlightedIndex >= 0
                          ? root.controllerHighlightedIndex : root.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }
}
