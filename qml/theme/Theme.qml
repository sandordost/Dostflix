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
    readonly property color button: "#2a2a2e"
    readonly property color buttonHover: "#38383e"
    readonly property color buttonPressed: "#45414f"
    readonly property color buttonText: "#f7f7fa"
    readonly property color separator: "#343438"
    readonly property string fontFamily: "Montserrat"
    readonly property string iconFontFamily: "Font Awesome 7 Free"
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

    function iconGlyph(name) {
        const icons = {
            "system-search-symbolic": "\uf002",
            "folder-videos-symbolic": "\uf008",
            "folder-download-symbolic": "\uf019",
            "preferences-system-symbolic": "\uf013",
            "go-previous-symbolic": "\uf060",
            "go-up-symbolic": "\uf062",
            "go-home-symbolic": "\uf015",
            "view-fullscreen-symbolic": "\uf065",
            "view-grid-symbolic": "\uf00a",
            "view-list-symbolic": "\uf03a",
            "media-playback-stop-symbolic": "\uf04d",
            "media-seek-backward-symbolic": "\uf048",
            "media-playback-start-symbolic": "\uf04b",
            "media-playback-pause-symbolic": "\uf04c",
            "media-seek-forward-symbolic": "\uf051",
            "audio-volume-high-symbolic": "\uf028",
            "view-refresh-symbolic": "\uf2f1",
            "document-open-symbolic": "\uf56f",
            "folder-open-symbolic": "\uf07c",
            "edit-delete-symbolic": "\uf1f8"
        }
        return icons[name] || ""
    }
}
