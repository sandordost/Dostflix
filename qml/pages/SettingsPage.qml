pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Dostflix

Item {
    id: root
    required property var vpnManager
    required property var providerManager
    required property var prowlarrManager
    required property var subtitleManager
    required property var libraryManager
    required property var controllerManager
    readonly property var settingsFlickable: settingsScroll.contentItem

    function navigationRows() {
        return [
            [profileBox, importProfileButton, refreshProfilesButton],
            [vpnConnectButton],
            [libraryPathField, chooseLibraryButton, refreshLibraryButton],
            [configureIndexersButton],
            [tmdbToken, saveTmdbButton, removeTmdbButton],
            [providerName, providerKind, providerEndpoint,
             providerApiKey, addProviderButton],
            [subtitleLanguages, saveLanguagesButton],
            [openSubtitlesApiKey, openSubtitlesUsername,
             openSubtitlesPassword, saveSubtitlesButton,
             removeSubtitlesButton]
        ]
    }

    function itemAvailable(item) {
        return item && item.visible && item.enabled && item.activeFocusOnTab
    }

    function itemContainsFocus(item, focusedItem) {
        let candidate = focusedItem
        while (candidate) {
            if (candidate === item)
                return true
            candidate = candidate.parent
        }
        return false
    }

    function focusFirstInRow(row) {
        for (let index = 0; index < row.length; ++index) {
            if (root.itemAvailable(row[index])) {
                row[index].forceActiveFocus(Qt.TabFocusReason)
                return true
            }
        }
        return false
    }

    function activeRowPosition(rows) {
        const focusedItem = root.Window.window
                ? root.Window.window.activeFocusItem : null
        for (let rowIndex = 0; rowIndex < rows.length; ++rowIndex) {
            for (let columnIndex = 0;
                 columnIndex < rows[rowIndex].length; ++columnIndex) {
                if (root.itemContainsFocus(rows[rowIndex][columnIndex], focusedItem))
                    return { row: rowIndex, column: columnIndex }
            }
        }
        return { row: -1, column: -1 }
    }

    function openControllerCombo() {
        if (profileBox.controllerPopupActive)
            return profileBox
        if (providerKind.controllerPopupActive)
            return providerKind
        return null
    }

    function activateControllerPopup() {
        const combo = root.openControllerCombo()
        if (!combo)
            return false
        combo.controllerActivate()
        return true
    }

    function handleControllerNavigation(horizontal, vertical) {
        const combo = root.openControllerCombo()
        if (combo) {
            if (vertical !== 0)
                combo.controllerNavigate(vertical)
            return true
        }

        const rows = root.navigationRows()
        const position = root.activeRowPosition(rows)
        if (position.row < 0)
            return false

        if (vertical !== 0) {
            for (let rowIndex = position.row + (vertical > 0 ? 1 : -1);
                 rowIndex >= 0 && rowIndex < rows.length;
                 rowIndex += vertical > 0 ? 1 : -1) {
                if (root.focusFirstInRow(rows[rowIndex]))
                    return true
            }
            return true
        }

        if (horizontal !== 0) {
            const row = rows[position.row]
            for (let columnIndex = position.column + (horizontal > 0 ? 1 : -1);
                 columnIndex >= 0 && columnIndex < row.length;
                 columnIndex += horizontal > 0 ? 1 : -1) {
                if (root.itemAvailable(row[columnIndex])) {
                    row[columnIndex].forceActiveFocus(Qt.TabFocusReason)
                    return true
                }
            }
            return true
        }
        return false
    }

    function focusFirstControl() {
        profileBox.forceActiveFocus(Qt.TabFocusReason)
        return true
    }

    function keepFocusVisible(item) {
        if (!item || !root.settingsFlickable || !item.visible)
            return
        const viewport = root.settingsFlickable
        let ancestor = item
        while (ancestor && ancestor !== viewport)
            ancestor = ancestor.parent
        if (ancestor !== viewport)
            return
        const point = item.mapToItem(viewport, 0, 0)
        const margin = Theme.px(24)
        if (point.y < margin)
            viewport.contentY = Math.max(0, viewport.contentY + point.y - margin)
        else if (point.y + item.height > viewport.height - margin)
            viewport.contentY = Math.min(viewport.contentHeight - viewport.height,
                    viewport.contentY + point.y + item.height - viewport.height + margin)
    }

    Connections {
        target: root.Window.window
        function onActiveFocusItemChanged() {
            root.keepFocusVisible(root.Window.window.activeFocusItem)
        }
    }

    PathPickerDialog {
        id: profileDialog
        title: qsTr("Choose an OpenVPN profile")
        fileNameFilters: ["*.ovpn"]
        onPathChosen: path => root.vpnManager.importProfile(path)
    }

    PathPickerDialog {
        id: libraryDialog
        title: qsTr("Choose the movie library folder")
        folderMode: true
        onPathChosen: path => root.libraryManager.setDirectory(path)
    }

    ScrollView {
        id: settingsScroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
        width: parent.width
        spacing: Theme.px(18)

        Label {
            text: qsTr("VPN protection")
            color: Theme.textPrimary
            font.pixelSize: Theme.headingSize
            font.weight: Font.DemiBold
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Dostflix connects the selected OpenVPN profile at startup and only disconnects connections it started itself.")
            color: Theme.textSecondary
            font.pixelSize: Theme.bodySize
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.px(10)

            AppComboBox {
                id: profileBox
                objectName: "profileBox"
                Layout.fillWidth: true
                model: root.vpnManager.profileModel
                textRole: "name"
                valueRole: "uuid"
                currentIndex: {
                    for (let index = 0; index < count; ++index) {
                        if (valueAt(index) === root.vpnManager.selectedProfileUuid)
                            return index
                    }
                    return -1
                }
                onActivated: root.vpnManager.selectProfile(currentValue)
                Accessible.name: qsTr("OpenVPN profile")
            }

            AppButton {
                id: importProfileButton
                objectName: "importProfileButton"
                text: qsTr("Import .ovpn…")
                icon.name: "document-open-symbolic"
                onClicked: profileDialog.openAt("")
            }

            AppToolButton {
                id: refreshProfilesButton
                objectName: "refreshProfilesButton"
                icon.name: "view-refresh-symbolic"
                icon.width: Theme.iconSize
                icon.height: Theme.iconSize
                onClicked: root.vpnManager.refreshProfiles()
                Accessible.name: qsTr("Refresh VPN profiles")
            }
        }

        RowLayout {
            spacing: Theme.px(10)

            Rectangle {
                implicitWidth: Theme.px(9)
                implicitHeight: Theme.px(9)
                radius: Theme.px(5)
                color: root.vpnManager.connected ? Theme.safe : Theme.textSecondary
            }

            Label {
                text: root.vpnManager.stateLabel
                color: Theme.textPrimary
                font.pixelSize: Theme.bodySize
            }

            AppButton {
                id: vpnConnectButton
                objectName: "vpnConnectButton"
                text: root.vpnManager.connected
                      ? (root.vpnManager.ownsConnection ? qsTr("Disconnect") : qsTr("Connected externally"))
                      : qsTr("Connect")
                enabled: root.vpnManager.selectedProfileUuid.length > 0
                         && !root.vpnManager.busy
                         && (!root.vpnManager.connected || root.vpnManager.ownsConnection)
                onClicked: root.vpnManager.connected
                           ? root.vpnManager.disconnectOwned()
                           : root.vpnManager.connectSelected()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.vpnManager.errorMessage.length > 0
            text: root.vpnManager.errorMessage
            color: Theme.danger
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: notice.implicitHeight + Theme.px(24)
            radius: Theme.radiusLarge
            color: Theme.surface

            Label {
                id: notice
                anchors.fill: parent
                anchors.margins: Theme.px(12)
                text: root.vpnManager.networkReady
                      ? qsTr("Network protection verified. Protected features may now use the VPN.")
                      : qsTr("Internet searches and downloads stay disabled until the kill switch has been installed and verified.")
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }
        }

        Label {
            text: qsTr("Controller")
            color: Theme.textPrimary
            font.pixelSize: Theme.headingSize
            font.weight: Font.DemiBold
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: controllerRow.implicitHeight + Theme.px(24)
            radius: Theme.radiusLarge
            color: Theme.surface

            RowLayout {
                id: controllerRow
                anchors.fill: parent
                anchors.margins: Theme.px(12)
                spacing: Theme.px(12)
                AppIcon {
                    iconName: "input-gaming-symbolic"
                    color: root.controllerManager.connected ? Theme.safe : Theme.textMuted
                    font.pixelSize: Theme.iconSizeLarge
                    Layout.preferredWidth: Theme.iconSizeLarge
                    Layout.preferredHeight: Theme.iconSizeLarge
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.px(3)
                    Label {
                        Layout.fillWidth: true
                        text: root.controllerManager.connected
                              ? root.controllerManager.controllerName
                              : qsTr("No controller connected")
                        color: Theme.textPrimary
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("D-pad/stick: spatial navigation · A/Cross: select · B/Circle: back or return to movie · Start: pause · triggers/shoulders: switch section or seek · X/Square: subtitles · Y/Triangle: search or fullscreen")
                        color: Theme.textSecondary
                        wrapMode: Text.WordWrap
                    }
                }
                Label {
                    visible: root.controllerManager.controllerCount > 1
                    text: qsTr("%1 connected").arg(root.controllerManager.controllerCount)
                    color: Theme.textSecondary
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.controllerManager.errorMessage.length > 0
            text: qsTr("Controller support unavailable: %1").arg(root.controllerManager.errorMessage)
            color: Theme.danger
            wrapMode: Text.WordWrap
        }

        Label {
            text: qsTr("Movie library")
            color: Theme.textPrimary
            font.pixelSize: Theme.headingSize
            font.weight: Font.DemiBold
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Dostflix scans this folder for existing local videos. Local playback does not require a VPN connection.")
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            AppTextField {
                id: libraryPathField
                objectName: "libraryPathField"
                Layout.fillWidth: true
                readOnly: true
                text: root.libraryManager.directory
                Accessible.name: qsTr("Movie library folder")
            }
            AppButton {
                id: chooseLibraryButton
                objectName: "chooseLibraryButton"
                text: qsTr("Choose folder…")
                icon.name: "folder-open-symbolic"
                onClicked: libraryDialog.openAt("file://" + root.libraryManager.directory)
            }
            AppToolButton {
                id: refreshLibraryButton
                objectName: "refreshLibraryButton"
                icon.name: "view-refresh-symbolic"
                icon.width: Theme.iconSize
                icon.height: Theme.iconSize
                Accessible.name: qsTr("Rescan movie library")
                onClicked: root.libraryManager.refresh()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.libraryManager.errorMessage.length > 0
            text: root.libraryManager.errorMessage
            color: Theme.danger
            wrapMode: Text.WordWrap
        }

        Label {
            text: qsTr("Torrent providers")
            color: Theme.textPrimary
            font.pixelSize: Theme.headingSize
            font.weight: Font.DemiBold
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: prowlarrRow.implicitHeight + Theme.px(24)
            radius: Theme.radiusLarge
            color: Theme.surface

            RowLayout {
                id: prowlarrRow
                anchors.fill: parent
                anchors.margins: Theme.px(12)
                spacing: Theme.px(10)

                BusyIndicator {
                    implicitWidth: Theme.px(22)
                    implicitHeight: Theme.px(22)
                    running: root.prowlarrManager.running && !root.prowlarrManager.ready
                    visible: running
                }
                Rectangle {
                    implicitWidth: Theme.px(9)
                    implicitHeight: Theme.px(9)
                    radius: Theme.px(5)
                    visible: !root.prowlarrManager.running || root.prowlarrManager.ready
                    color: root.prowlarrManager.ready ? Theme.safe : Theme.textSecondary
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.px(2)
                    Label { text: qsTr("Managed Prowlarr"); color: Theme.textPrimary; font.weight: Font.DemiBold }
                    Label { text: root.prowlarrManager.stateLabel; color: Theme.textSecondary }
                }
                AppButton {
                    id: configureIndexersButton
                    objectName: "configureIndexersButton"
                    text: qsTr("Configure indexers")
                    enabled: root.prowlarrManager.ready
                    onClicked: root.prowlarrManager.openWebInterface()
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.prowlarrManager.errorMessage.length > 0
            text: root.prowlarrManager.errorMessage
            color: Theme.danger
            wrapMode: Text.WordWrap
        }

        Label {
            text: qsTr("Movie metadata")
            color: Theme.textPrimary
            font.pixelSize: Theme.headingSize
            font.weight: Font.DemiBold
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Add your optional TMDB API Read Access Token for movie posters and metadata. It is stored in a private user-only credential file.")
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            AppTextField {
                id: tmdbToken
                objectName: "tmdbTokenField"
                Layout.fillWidth: true
                placeholderText: root.providerManager.hasTmdbToken
                                 ? qsTr("TMDB token saved")
                                 : qsTr("TMDB API Read Access Token")
                echoMode: TextInput.Password
            }
            AppButton {
                id: saveTmdbButton
                objectName: "saveTmdbButton"
                text: qsTr("Save token")
                onClicked: {
                    if (root.providerManager.saveTmdbToken(tmdbToken.text))
                        tmdbToken.clear()
                }
            }
            AppButton {
                id: removeTmdbButton
                objectName: "removeTmdbButton"
                text: qsTr("Remove")
                visible: root.providerManager.hasTmdbToken
                onClicked: root.providerManager.clearTmdbToken()
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Configure indexers in managed Prowlarr above, or add a separate Torznab-compatible endpoint. Dostflix includes no indexers.")
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            AppTextField {
                id: providerName
                objectName: "providerNameField"
                Layout.preferredWidth: Theme.px(190)
                placeholderText: qsTr("Provider name")
            }
            AppComboBox {
                id: providerKind
                objectName: "providerKindBox"
                model: ["Torznab", "Prowlarr"]
            }
            AppTextField {
                id: providerEndpoint
                objectName: "providerEndpointField"
                Layout.fillWidth: true
                placeholderText: qsTr("https://example.test/api")
            }
            AppTextField {
                id: providerApiKey
                objectName: "providerApiKeyField"
                Layout.preferredWidth: Theme.px(190)
                placeholderText: qsTr("API key (optional)")
                echoMode: TextInput.Password
            }
            AppButton {
                id: addProviderButton
                objectName: "addProviderButton"
                text: qsTr("Add provider")
                onClicked: {
                    if (root.providerManager.addProvider(providerName.text,
                                                         providerKind.currentText,
                                                         providerEndpoint.text,
                                                         providerApiKey.text)) {
                        providerName.clear()
                        providerEndpoint.clear()
                        providerApiKey.clear()
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.providerManager.errorMessage.length > 0
            text: root.providerManager.errorMessage
            color: Theme.danger
            wrapMode: Text.WordWrap
        }

        ListView {
            Layout.fillWidth: true
            implicitHeight: Math.min(contentHeight, Theme.px(150))
            model: root.providerManager.model
            spacing: Theme.px(6)
            clip: true
            delegate: Rectangle {
                id: providerDelegate
                required property int index
                required property string name
                required property string kind
                required property string endpoint
                width: ListView.view.width
                height: Theme.px(52)
                radius: Theme.radiusLarge
                color: Theme.surface
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.px(10)
                    Label { text: providerDelegate.name; color: Theme.textPrimary; font.weight: Font.DemiBold }
                    Label { text: providerDelegate.kind; color: Theme.textSecondary }
                    Label { Layout.fillWidth: true; text: providerDelegate.endpoint; color: Theme.textSecondary; elide: Text.ElideMiddle }
                    AppToolButton {
                        icon.name: "edit-delete-symbolic"
                        Accessible.name: qsTr("Remove provider")
                        onClicked: root.providerManager.removeProvider(providerDelegate.index)
                    }
                }
            }
        }

        Label {
            text: qsTr("OpenSubtitles")
            color: Theme.textPrimary
            font.pixelSize: Theme.headingSize
            font.weight: Font.DemiBold
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Add your OpenSubtitles.com API key and account. Credentials are stored together in the desktop secret store; login tokens remain in memory only.")
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Preferred languages")
                color: Theme.textPrimary
            }
            AppTextField {
                id: subtitleLanguages
                objectName: "subtitleLanguagesField"
                Layout.preferredWidth: Theme.px(220)
                text: root.subtitleManager.preferredLanguages
                placeholderText: qsTr("nl,en")
                Accessible.name: qsTr("Preferred subtitle language codes")
                onAccepted: root.subtitleManager.setPreferredLanguages(text)
            }
            AppButton {
                id: saveLanguagesButton
                objectName: "saveLanguagesButton"
                text: qsTr("Save languages")
                onClicked: root.subtitleManager.setPreferredLanguages(subtitleLanguages.text)
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Comma-separated ISO codes, in preference order.")
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }
        }

        RowLayout {
            Layout.fillWidth: true
            AppTextField {
                id: openSubtitlesApiKey
                objectName: "openSubtitlesApiKeyField"
                Layout.fillWidth: true
                placeholderText: root.subtitleManager.configured
                                 ? qsTr("OpenSubtitles API key saved")
                                 : qsTr("API key")
                echoMode: TextInput.Password
            }
            AppTextField {
                id: openSubtitlesUsername
                objectName: "openSubtitlesUsernameField"
                Layout.preferredWidth: Theme.px(210)
                placeholderText: root.subtitleManager.username.length > 0
                                 ? root.subtitleManager.username : qsTr("Username")
            }
            AppTextField {
                id: openSubtitlesPassword
                objectName: "openSubtitlesPasswordField"
                Layout.preferredWidth: Theme.px(210)
                placeholderText: qsTr("Password")
                echoMode: TextInput.Password
            }
            AppButton {
                id: saveSubtitlesButton
                objectName: "saveSubtitlesButton"
                text: qsTr("Save")
                onClicked: {
                    if (root.subtitleManager.saveCredentials(openSubtitlesApiKey.text,
                                                             openSubtitlesUsername.text,
                                                             openSubtitlesPassword.text)) {
                        openSubtitlesApiKey.clear()
                        openSubtitlesUsername.clear()
                        openSubtitlesPassword.clear()
                    }
                }
            }
            AppButton {
                id: removeSubtitlesButton
                objectName: "removeSubtitlesButton"
                text: qsTr("Remove")
                visible: root.subtitleManager.configured
                onClicked: root.subtitleManager.clearCredentials()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.subtitleManager.errorMessage.length > 0
            text: root.subtitleManager.errorMessage
            color: Theme.danger
            wrapMode: Text.WordWrap
        }

        Item { implicitHeight: 1 }
        }
    }
}
