pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    property int currentIndex: 0
    property bool searchEnabled: false
    property bool compact: false
    property string pendingSearch: ""
    signal pageRequested(int index)
    signal searchRequested(string query)
    implicitWidth: compact ? Theme.sidebarCompactWidth : Theme.sidebarWidth

    function queueSearch(query) {
        pendingSearch = query.trim()
        searchDebounce.stop()
        if (pendingSearch.length > 0 && searchEnabled) {
            pageRequested(0)
            searchDebounce.start()
        }
    }

    Timer {
        id: searchDebounce
        interval: 2000
        repeat: false
        onTriggered: {
            if (root.pendingSearch.length > 0 && root.searchEnabled)
                root.searchRequested(root.pendingSearch)
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusLarge
        color: Qt.rgba(0.071, 0.071, 0.078, Theme.panelOpacity)
    }

    Popup {
        id: compactSearch
        x: root.width + 10
        y: 0
        width: 320
        height: 62
        padding: 10
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: compactSearchField.forceActiveFocus()
        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
        }
        contentItem: TextField {
            id: compactSearchField
            placeholderText: qsTr("Search movies…")
            enabled: root.searchEnabled
            leftPadding: 14
            rightPadding: 14
            font.family: Theme.fontFamily
            font.pixelSize: Theme.bodySize
            onTextChanged: root.queueSearch(text)
            onAccepted: {
                root.queueSearch(text)
                compactSearch.close()
            }
            background: Rectangle {
                radius: Theme.radiusSmall
                color: Theme.input
                border.width: compactSearchField.activeFocus ? 2 : 0
                border.color: Theme.accent
            }
            Accessible.name: qsTr("Search movies")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.compact ? 10 : 14
        spacing: 8

        TextField {
            id: searchField
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            visible: !root.compact
            placeholderText: qsTr("Search movies…")
            enabled: root.searchEnabled
            leftPadding: 14
            rightPadding: 14
            font.family: Theme.fontFamily
            font.pixelSize: Theme.bodySize
            onTextChanged: root.queueSearch(text)
            background: Rectangle {
                radius: Theme.radiusSmall
                color: Theme.input
                border.width: searchField.activeFocus ? 2 : 0
                border.color: Theme.accent
            }
            Accessible.name: qsTr("Search movies")
        }

        ToolButton {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            id: compactSearchButton
            visible: root.compact
            icon.name: "system-search-symbolic"
            icon.width: Theme.iconSize
            icon.height: Theme.iconSize
            enabled: root.searchEnabled
            onClicked: compactSearch.open()
            background: Rectangle {
                radius: Theme.radiusSmall
                color: compactSearchButton.hovered ? Theme.raisedHover : Theme.input
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
            }
            Accessible.name: qsTr("Search movies")
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            visible: !root.compact
            color: Theme.separator
        }

        Repeater {
            model: [
                { label: qsTr("Discover"), iconName: "system-search-symbolic" },
                { label: qsTr("Library"), iconName: "folder-videos-symbolic" },
                { label: qsTr("Downloads"), iconName: "folder-download-symbolic" },
                { label: qsTr("Settings"), iconName: "preferences-system-symbolic" }
            ]

            delegate: ToolButton {
                id: navButton
                required property int index
                required property var modelData
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                text: root.compact ? "" : modelData.label
                icon.name: modelData.iconName
                icon.width: Theme.iconSize
                icon.height: Theme.iconSize
                display: root.compact ? AbstractButton.IconOnly : AbstractButton.TextBesideIcon
                spacing: 12
                font.family: Theme.fontFamily
                font.pixelSize: Theme.bodySize
                font.weight: checked ? Font.DemiBold : Font.Medium
                checkable: true
                checked: root.currentIndex === index
                focusPolicy: Qt.TabFocus
                onClicked: root.pageRequested(index)
                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: navButton.checked ? Theme.raised
                                             : (navButton.hovered ? Theme.surface : "transparent")
                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: 3
                        height: 24
                        radius: 2
                        visible: navButton.checked
                        color: Theme.accent
                    }
                    Behavior on color { ColorAnimation { duration: Theme.motionFast } }
                }
                ToolTip.visible: root.compact && hovered
                ToolTip.text: modelData.label
                Accessible.name: modelData.label
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label {
                text: "●"
                color: root.searchEnabled ? Theme.safe : Theme.textMuted
                font.pixelSize: 10
            }
            Label {
                Layout.fillWidth: true
                visible: !root.compact
                text: root.searchEnabled
                      ? qsTr("Protected search ready")
                      : qsTr("Connect VPN to search")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.captionSize
                wrapMode: Text.WordWrap
            }
        }
    }
}
