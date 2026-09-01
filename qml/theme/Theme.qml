pragma Singleton
import QtQuick

QtObject {
    readonly property color canvas: "#020307"
    readonly property color panel: "#121214"
    readonly property real panelOpacity: 0.91
    readonly property color surface: "#1b1b1e"
    readonly property color raised: "#29292d"
    readonly property color raisedHover: "#343439"
    readonly property color input: "#202023"
    readonly property color textPrimary: "#f7f7fa"
    readonly property color textSecondary: "#aaaab1"
    readonly property color textMuted: "#74747c"
    readonly property color accent: "#756cff"
    readonly property color accentSoft: "#332e78"
    readonly property color purple: accent
    readonly property color blue: "#4f86ff"
    readonly property color safe: "#70e6aa"
    readonly property color warning: "#f4c76b"
    readonly property color danger: "#ff9299"
    readonly property color button: "#e8e6eb"
    readonly property color buttonText: "#202124"
    readonly property color separator: "#343438"
    readonly property string fontFamily: "Noto Sans"
    readonly property int radiusSmall: 6
    readonly property int radius: 10
    readonly property int radiusLarge: 14
    readonly property int iconSize: 20
    readonly property int iconSizeLarge: 28
    readonly property int captionSize: 12
    readonly property int bodySize: 14
    readonly property int headingSize: 22
    readonly property int titleSize: 28
    readonly property int headerHeight: 78
    readonly property int sidebarWidth: 252
    readonly property int sidebarCompactWidth: 76
    readonly property int contentGap: 12
    readonly property int pagePadding: 22
    readonly property int motionFast: 120
    readonly property int motionNormal: 180
    readonly property int controlsTimeout: 2800
    readonly property int posterWidth: 170
    readonly property real posterAspectRatio: 2 / 3
}
