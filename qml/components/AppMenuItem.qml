import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

MenuItem {
    id: root
    implicitHeight: Theme.px(40)
    leftPadding: Theme.px(12)
    rightPadding: Theme.px(12)
    font.family: Theme.fontFamily
    font.pixelSize: Theme.bodySize

    indicator: Item { implicitWidth: 0; implicitHeight: 0 }

    contentItem: RowLayout {
        spacing: Theme.px(9)
        AppIcon {
            visible: root.checkable
            glyph: root.checked ? "\uf00c" : ""
            color: Theme.textPrimary
            font.pixelSize: Theme.px(13)
            Layout.preferredWidth: Theme.px(16)
        }
        Label {
            Layout.fillWidth: true
            text: root.text
            color: root.enabled ? Theme.textPrimary : Theme.textMuted
            font: root.font
            elide: Text.ElideRight
        }
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: root.highlighted || root.hovered ? Theme.raisedHover : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
    }
}
