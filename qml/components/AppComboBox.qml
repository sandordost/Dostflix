pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Dostflix

ComboBox {
    id: root
    implicitHeight: Theme.px(44)
    leftPadding: Theme.px(14)
    rightPadding: Theme.px(42)
    font.family: Theme.fontFamily
    font.pixelSize: Theme.bodySize
    palette.text: Theme.textPrimary
    palette.buttonText: Theme.textPrimary
    palette.highlight: Theme.accentSoft
    palette.highlightedText: Theme.textPrimary

    function controllerActivate() {
        if (root.popup.opened)
            root.popup.close()
        else
            root.popup.open()
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
        highlighted: root.highlightedIndex === index
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
        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
        }
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }
}
