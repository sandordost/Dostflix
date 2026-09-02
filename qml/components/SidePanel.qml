pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Dostflix

Item {
    id: root
    property int currentIndex: 0
    property bool searchEnabled: false
    property bool compact: false
    property string pendingSearch: ""
    required property var controllerManager
    property bool controllerSearchActive: false
    property bool controllerSearchHadQuery: false
    property var controllerPreviousFocus: null
    signal pageRequested(int index)
    signal searchRequested(string query)
    signal controllerSearchDismissed(bool searched)
    implicitWidth: compact ? Theme.sidebarCompactWidth : Theme.sidebarWidth

    function queueSearch(query) {
        pendingSearch = query.trim()
        if (controllerSearchActive && pendingSearch.length > 0)
            controllerSearchHadQuery = true
        searchDebounce.stop()
        if (pendingSearch.length > 0 && searchEnabled) {
            pageRequested(0)
            searchDebounce.start()
        }
    }

    function openControllerSearch() {
        if (!searchEnabled)
            return
        controllerPreviousFocus = root.Window.window
                ? root.Window.window.activeFocusItem : null
        controllerSearchActive = true
        controllerSearchHadQuery = false
        if (compact) {
            compactSearch.open()
        } else {
            searchField.forceActiveFocus(Qt.ShortcutFocusReason)
            searchField.selectAll()
        }
    }

    function closeControllerSearch() {
        if (!controllerSearchActive)
            return false
        const searched = controllerSearchHadQuery
        controllerSearchActive = false
        if (compactSearch.opened)
            compactSearch.close()
        controllerSearchDismissed(searched)
        controllerSearchHadQuery = false
        return true
    }

    function restoreControllerFocus() {
        if (controllerPreviousFocus && controllerPreviousFocus.visible
                && controllerPreviousFocus.enabled) {
            controllerPreviousFocus.forceActiveFocus(Qt.BacktabFocusReason)
            return true
        }
        return false
    }

    function focusCurrentPage() {
        const button = navRepeater.itemAt(currentIndex)
        if (button)
            button.forceActiveFocus(Qt.TabFocusReason)
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
        x: root.width + Theme.px(10)
        y: 0
        width: Theme.px(320)
        height: Theme.px(62)
        padding: Theme.px(10)
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onOpened: compactSearchField.forceActiveFocus()
        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
        }
        contentItem: AppTextField {
            id: compactSearchField
            objectName: "compactSearchField"
            placeholderText: qsTr("Search movies…")
            enabled: root.searchEnabled
            activeFocusOnTab: false
            focusPolicy: Qt.StrongFocus
            leftPadding: Theme.px(14)
            rightPadding: Theme.px(14)
            font.family: Theme.fontFamily
            font.pixelSize: Theme.bodySize
            onTextChanged: root.queueSearch(text)
            onAccepted: {
                root.queueSearch(text)
                compactSearch.close()
            }
            background: Rectangle {
                radius: Theme.radius
                color: compactSearchField.activeFocus ? Theme.raisedHover : Theme.input
            }
            Accessible.name: qsTr("Search movies")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.compact ? Theme.px(10) : Theme.px(14)
        spacing: Theme.px(8)

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumHeight: Theme.px(46)
            Layout.preferredHeight: Theme.px(46)
            Layout.maximumHeight: Theme.px(46)
            visible: !root.compact
            spacing: Theme.px(8)

            AppTextField {
                id: searchField
                objectName: "searchField"
                Layout.fillWidth: true
                Layout.minimumHeight: Theme.px(46)
                Layout.preferredHeight: Theme.px(46)
                Layout.maximumHeight: Theme.px(46)
                placeholderText: qsTr("Search movies…")
                enabled: root.searchEnabled
                activeFocusOnTab: false
                focusPolicy: Qt.StrongFocus
                leftPadding: Theme.px(14)
                rightPadding: Theme.px(14)
                font.family: Theme.fontFamily
                font.pixelSize: Theme.bodySize
                onTextChanged: root.queueSearch(text)
                background: Rectangle {
                    radius: Theme.radius
                    color: searchField.activeFocus ? Theme.raisedHover : Theme.input
                }
                Accessible.name: qsTr("Search movies")
            }

            ControllerHint {
                visible: root.controllerManager.connected
                buttonLabel: root.controllerManager.searchButtonLabel
                description: qsTr("Focus search")
            }
        }

        AppToolButton {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.px(46)
            id: compactSearchButton
            visible: root.compact
            icon.name: "system-search-symbolic"
            icon.width: Theme.iconSize
            icon.height: Theme.iconSize
            enabled: root.searchEnabled
            activeFocusOnTab: false
            focusPolicy: Qt.NoFocus
            onClicked: compactSearch.open()
            background: Rectangle {
                radius: Theme.radiusSmall
                color: compactSearchButton.hovered ? Theme.raisedHover : Theme.input
                Behavior on color { ColorAnimation { duration: Theme.motionFast } }
            }
            Accessible.name: qsTr("Search movies")

            ControllerHint {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: -Theme.px(3)
                visible: root.controllerManager.connected
                buttonLabel: root.controllerManager.searchButtonLabel
                description: qsTr("Focus search")
                scale: 0.78
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.px(1)
            visible: !root.compact
            color: Theme.separator
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.controllerManager.connected
            spacing: Theme.px(7)

            ControllerHint {
                buttonLabel: root.controllerManager.previousPageLabel
                description: qsTr("Previous section")
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Switch section")
                color: Theme.textMuted
                font.pixelSize: Theme.captionSize
                horizontalAlignment: Text.AlignHCenter
            }
            ControllerHint {
                buttonLabel: root.controllerManager.nextPageLabel
                description: qsTr("Next section")
            }
        }

        Repeater {
            id: navRepeater
            model: [
                { label: qsTr("Discover"), iconName: "system-search-symbolic" },
                { label: qsTr("Library"), iconName: "folder-videos-symbolic" },
                { label: qsTr("Downloads"), iconName: "folder-download-symbolic" },
                { label: qsTr("Settings"), iconName: "preferences-system-symbolic" }
            ]

            delegate: AppButton {
                id: navButton
                required property int index
                required property var modelData
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.px(48)
                text: root.compact ? "" : modelData.label
                icon.name: modelData.iconName
                icon.width: Theme.iconSize
                icon.height: Theme.iconSize
                display: root.compact ? AbstractButton.IconOnly : AbstractButton.TextBesideIcon
                alignLeft: !root.compact
                leftPadding: root.compact ? Theme.px(11) : Theme.px(24)
                rightPadding: root.compact ? Theme.px(11) : Theme.px(16)
                spacing: Theme.px(12)
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
                        width: Theme.px(3)
                        height: Theme.px(24)
                        radius: Theme.px(2)
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
            spacing: Theme.px(8)
            Label {
                text: "●"
                color: root.searchEnabled ? Theme.safe : Theme.textMuted
                font.pixelSize: Theme.px(10)
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
