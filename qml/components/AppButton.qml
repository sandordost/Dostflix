import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Button {
    id: root
    property string symbol: ""
    property bool primary: false
    property bool destructive: false
    property bool quiet: false
    property int cornerRadius: Theme.radius

    implicitHeight: 42
    implicitWidth: Math.max(42, contentRow.implicitWidth + leftPadding + rightPadding)
    leftPadding: text.length > 0 ? 16 : 11
    rightPadding: text.length > 0 ? 16 : 11
    spacing: 9
    font.family: Theme.fontFamily
    font.pixelSize: Theme.bodySize
    palette.buttonText: enabled ? Theme.textPrimary : Theme.textMuted
    focusPolicy: Qt.StrongFocus

    contentItem: RowLayout {
        id: contentRow
        spacing: root.spacing

        AppIcon {
            visible: text.length > 0
            glyph: root.symbol
            iconName: root.icon.name
            color: root.enabled ? Theme.textPrimary : Theme.textMuted
            font.pixelSize: root.icon.width > 0 ? root.icon.width : Theme.iconSize
            Layout.preferredWidth: visible ? Math.max(font.pixelSize, implicitWidth) : 0
            Layout.alignment: Qt.AlignVCenter
        }
        Label {
            visible: root.text.length > 0
            text: root.text
            color: root.enabled ? Theme.textPrimary : Theme.textMuted
            font: root.font
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            Layout.alignment: Qt.AlignVCenter
        }
    }

    background: Rectangle {
        radius: root.cornerRadius
        color: {
            if (!root.enabled)
                return Qt.rgba(0.16, 0.16, 0.18, 0.55)
            if (root.down)
                return root.primary ? Theme.accentSoft : Theme.buttonPressed
            if (root.hovered)
                return root.primary ? Qt.lighter(Theme.accent, 1.10) : Theme.buttonHover
            if (root.primary)
                return Theme.accent
            if (root.destructive)
                return Qt.rgba(0.55, 0.16, 0.18, 0.82)
            return root.quiet ? "transparent" : Theme.button
        }
        Behavior on color { ColorAnimation { duration: Theme.motionFast } }
    }
}
