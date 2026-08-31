pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    property int currentIndex: 0
    property bool searchEnabled: false
    signal pageRequested(int index)
    signal searchRequested(string query)

    Timer {
        id: searchDebounce
        interval: 2000
        repeat: false
        onTriggered: {
            if (searchField.text.trim().length > 0 && root.searchEnabled)
                root.searchRequested(searchField.text)
        }
    }
    implicitWidth: 255

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: Qt.rgba(0.071, 0.071, 0.078, Theme.panelOpacity)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 6

        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: qsTr("Search movies…")
            enabled: root.searchEnabled
            onTextChanged: {
                searchDebounce.stop()
                if (text.trim().length > 0 && root.searchEnabled) {
                    root.pageRequested(0)
                    searchDebounce.start()
                }
            }
            Accessible.name: qsTr("Search movies")
        }

        Repeater {
            model: [
                { label: qsTr("Discover"), iconName: "system-search" },
                { label: qsTr("Library"), iconName: "folder-videos-symbolic" },
                { label: qsTr("Downloads"), iconName: "folder-download-symbolic" },
                { label: qsTr("Settings"), iconName: "preferences-system-symbolic" }
            ]

            delegate: ToolButton {
                required property int index
                required property var modelData
                Layout.fillWidth: true
                text: modelData.label
                icon.name: modelData.iconName
                icon.width: Theme.iconSize
                icon.height: Theme.iconSize
                checkable: true
                checked: root.currentIndex === index
                onClicked: root.pageRequested(index)
                Accessible.name: modelData.label
            }
        }

        Item { Layout.fillHeight: true }

        Label {
            Layout.fillWidth: true
            text: qsTr("VPN protection required for network features")
            color: Theme.textSecondary
            font.pixelSize: 11
            wrapMode: Text.WordWrap
        }
    }
}
