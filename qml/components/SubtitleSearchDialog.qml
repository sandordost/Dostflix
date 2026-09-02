pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Dialog {
    id: root
    required property var manager
    property string query: ""
    property var controllerResultButtons: []
    signal settingsRequested()
    anchors.centerIn: parent
    width: Math.min(Theme.px(760), parent ? parent.width - Theme.px(80) : Theme.px(760))
    height: Math.min(Theme.px(580), parent ? parent.height - Theme.px(80) : Theme.px(580))
    modal: true
    title: qsTr("Find subtitles")
    background: Rectangle { radius: Theme.radiusLarge; color: Theme.panel }

    function registerResultButton(button) {
        if (controllerResultButtons.indexOf(button) < 0)
            controllerResultButtons = controllerResultButtons.concat([button])
    }

    function unregisterResultButton(button) {
        controllerResultButtons = controllerResultButtons.filter(
                    candidate => candidate !== button)
    }

    function navigationRows() {
        const rows = [[searchField, searchButton]]
        if (settingsButton.visible)
            rows.push([settingsButton])
        const sortedButtons = controllerResultButtons.slice().sort(
                    (left, right) => left.resultIndex - right.resultIndex)
        for (let index = 0; index < sortedButtons.length; ++index)
            rows.push([sortedButtons[index]])
        rows.push([closeButton])
        return rows
    }

    function itemAvailable(item) {
        return item && item.visible && item.enabled && item.activeFocusOnTab
    }

    function focusFirstInRow(row) {
        for (let index = 0; index < row.length; ++index) {
            if (itemAvailable(row[index])) {
                row[index].forceActiveFocus(Qt.TabFocusReason)
                return true
            }
        }
        return false
    }

    function activeRowPosition(rows) {
        for (let rowIndex = 0; rowIndex < rows.length; ++rowIndex) {
            for (let columnIndex = 0; columnIndex < rows[rowIndex].length;
                 ++columnIndex) {
                if (rows[rowIndex][columnIndex].activeFocus)
                    return { row: rowIndex, column: columnIndex }
            }
        }
        return { row: -1, column: -1 }
    }

    function focusFirstControl() {
        searchField.forceActiveFocus(Qt.PopupFocusReason)
        return true
    }

    function handleControllerNavigation(horizontal, vertical) {
        const rows = navigationRows()
        const position = activeRowPosition(rows)
        if (position.row < 0)
            return focusFirstControl()
        if (vertical !== 0) {
            for (let rowIndex = position.row + (vertical > 0 ? 1 : -1);
                 rowIndex >= 0 && rowIndex < rows.length;
                 rowIndex += vertical > 0 ? 1 : -1) {
                if (focusFirstInRow(rows[rowIndex]))
                    return true
            }
            return true
        }
        if (horizontal !== 0) {
            const row = rows[position.row]
            for (let columnIndex = position.column + (horizontal > 0 ? 1 : -1);
                 columnIndex >= 0 && columnIndex < row.length;
                 columnIndex += horizontal > 0 ? 1 : -1) {
                if (itemAvailable(row[columnIndex])) {
                    row[columnIndex].forceActiveFocus(Qt.TabFocusReason)
                    return true
                }
            }
            return true
        }
        return false
    }

    footer: DialogButtonBox {
        background: Item {}
        AppButton {
            id: closeButton
            objectName: "subtitleSearchCloseButton"
            text: qsTr("Close")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: root.reject()
        }
    }

    onOpened: {
        if (manager.configured && manager.networkReady)
            manager.search(query)
        Qt.callLater(focusFirstControl)
    }
    onClosed: manager.cancel()

    contentItem: ColumnLayout {
        spacing: Theme.px(12)

        RowLayout {
            Layout.fillWidth: true
            AppTextField {
                id: searchField
                objectName: "subtitleSearchField"
                Layout.fillWidth: true
                text: root.query
                placeholderText: qsTr("Movie or release title")
                onAccepted: root.manager.search(text)
            }
            AppButton {
                id: searchButton
                objectName: "subtitleSearchButton"
                text: qsTr("Search")
                primary: true
                enabled: !root.manager.busy && root.manager.configured && root.manager.networkReady
                onClicked: root.manager.search(searchField.text)
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !root.manager.configured
            text: qsTr("Configure your OpenSubtitles account in Settings first.")
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }
        AppButton {
            id: settingsButton
            objectName: "subtitleSettingsButton"
            visible: !root.manager.configured
            text: qsTr("Open Settings")
            onClicked: root.settingsRequested()
        }
        Label {
            Layout.fillWidth: true
            visible: root.manager.configured && !root.manager.networkReady
            text: qsTr("VPN protection must be connected before searching OpenSubtitles.")
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }
        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: root.manager.busy
            visible: running
        }
        Label {
            Layout.fillWidth: true
            visible: root.manager.statusLabel.length > 0
            text: root.manager.statusLabel
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            visible: root.manager.errorMessage.length > 0
            text: root.manager.errorMessage
            color: "#ff9b9b"
            wrapMode: Text.WordWrap
        }

        ListView {
            id: resultsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.px(6)
            model: root.manager.results
            delegate: Rectangle {
                id: resultDelegate
                required property int index
                required property var modelData
                width: ListView.view.width
                height: Theme.px(70)
                radius: Theme.radius
                color: Theme.raised
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.px(10)
                    spacing: Theme.px(10)
                    Label {
                        text: resultDelegate.modelData.language.toUpperCase()
                        color: Theme.textPrimary
                        font.weight: Font.Bold
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.px(2)
                        Label {
                            Layout.fillWidth: true
                            text: resultDelegate.modelData.release
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                        }
                        Label {
                            text: qsTr("%1 downloads%2%3")
                                  .arg(resultDelegate.modelData.downloads)
                                  .arg(resultDelegate.modelData.trusted ? qsTr(" · trusted") : "")
                                  .arg(resultDelegate.modelData.hearingImpaired ? qsTr(" · hearing impaired") : "")
                            color: Theme.textSecondary
                        }
                    }
                    AppButton {
                        id: downloadButton
                        property int resultIndex: resultDelegate.index
                        objectName: "subtitleDownloadButton-" + resultIndex
                        text: qsTr("Download")
                        primary: true
                        enabled: !root.manager.busy
                        onClicked: root.manager.download(resultDelegate.index)
                        onActiveFocusChanged: {
                            if (activeFocus)
                                resultsList.positionViewAtIndex(resultIndex,
                                                               ListView.Contain)
                        }
                        Component.onCompleted: root.registerResultButton(downloadButton)
                        Component.onDestruction: root.unregisterResultButton(downloadButton)
                    }
                }
            }
        }
    }
}
