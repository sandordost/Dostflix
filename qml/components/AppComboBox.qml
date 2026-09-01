pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Dostflix

ComboBox {
    id: root
    implicitHeight: 44
    leftPadding: 14
    rightPadding: 42
    font.family: Theme.fontFamily
    font.pixelSize: Theme.bodySize
    palette.text: Theme.textPrimary
    palette.buttonText: Theme.textPrimary
    palette.highlight: Theme.accentSoft
    palette.highlightedText: Theme.textPrimary

    contentItem: Text {
        text: root.displayText
        color: root.enabled ? Theme.textPrimary : Theme.textMuted
        font: root.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: AppIcon {
        x: root.width - width - 14
        y: (root.height - height) / 2
        glyph: "\uf078"
        color: Theme.textSecondary
        font.pixelSize: 13
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
        height: 40
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
        y: root.height + 6
        width: root.width
        implicitHeight: Math.min(contentItem.implicitHeight + 12, 320)
        padding: 6
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
