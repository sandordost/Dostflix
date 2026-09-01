import QtQuick
import QtQuick.Controls
import Dostflix

Label {
    id: root
    property string glyph: ""
    property string iconName: ""
    text: glyph.length > 0 ? glyph : Theme.iconGlyph(iconName)
    color: Theme.textPrimary
    font.family: Theme.iconFontFamily
    font.styleName: "Solid"
    font.pixelSize: Theme.iconSize
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
}
