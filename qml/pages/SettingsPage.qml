pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property var vpnManager
    required property var providerManager
    required property var prowlarrManager
    required property var subtitleManager
    required property var libraryManager

    FileDialog {
        id: profileDialog
        title: qsTr("Choose an OpenVPN profile")
        nameFilters: [qsTr("OpenVPN profiles (*.ovpn)")]
        onAccepted: root.vpnManager.importProfile(selectedFile)
    }

    FolderDialog {
        id: libraryDialog
        title: qsTr("Choose the movie library folder")
        currentFolder: "file://" + root.libraryManager.directory
        onAccepted: root.libraryManager.setDirectory(selectedFolder)
    }

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
        width: parent.width
        spacing: 18

        Label {
            text: qsTr("VPN protection")
            color: Theme.textPrimary
            font.pixelSize: Theme.titleSize
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
            spacing: 10

            ComboBox {
                id: profileBox
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

            Button {
                text: qsTr("Import .ovpn…")
                icon.name: "document-open-symbolic"
                onClicked: profileDialog.open()
            }

            ToolButton {
                icon.name: "view-refresh-symbolic"
                icon.width: Theme.iconSize
                icon.height: Theme.iconSize
                onClicked: root.vpnManager.refreshProfiles()
                Accessible.name: qsTr("Refresh VPN profiles")
            }
        }

        RowLayout {
            spacing: 10

            Rectangle {
                implicitWidth: 9
                implicitHeight: 9
                radius: 5
                color: root.vpnManager.connected ? Theme.safe : Theme.textSecondary
            }

            Label {
                text: root.vpnManager.stateLabel
                color: Theme.textPrimary
                font.pixelSize: Theme.bodySize
            }

            Button {
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
            color: "#ff9b9b"
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: notice.implicitHeight + 24
            radius: Theme.radius
            color: Theme.raised

            Label {
                id: notice
                anchors.fill: parent
                anchors.margins: 12
                text: root.vpnManager.networkReady
                      ? qsTr("Network protection verified. Protected features may now use the VPN.")
                      : qsTr("Internet searches and downloads stay disabled until the kill switch has been installed and verified.")
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }
        }

        Label {
            text: qsTr("Movie library")
            color: Theme.textPrimary
            font.pixelSize: Theme.titleSize
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
            TextField {
                Layout.fillWidth: true
                readOnly: true
                text: root.libraryManager.directory
                Accessible.name: qsTr("Movie library folder")
            }
            Button {
                text: qsTr("Choose folder…")
                icon.name: "folder-open-symbolic"
                onClicked: libraryDialog.open()
            }
            ToolButton {
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
            color: "#ff9b9b"
            wrapMode: Text.WordWrap
        }

        Label {
            text: qsTr("Torrent providers")
            color: Theme.textPrimary
            font.pixelSize: Theme.titleSize
            font.weight: Font.DemiBold
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: prowlarrRow.implicitHeight + 24
            radius: Theme.radius
            color: Theme.raised

            RowLayout {
                id: prowlarrRow
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                BusyIndicator {
                    implicitWidth: 22
                    implicitHeight: 22
                    running: root.prowlarrManager.running && !root.prowlarrManager.ready
                    visible: running
                }
                Rectangle {
                    implicitWidth: 9
                    implicitHeight: 9
                    radius: 5
                    visible: !root.prowlarrManager.running || root.prowlarrManager.ready
                    color: root.prowlarrManager.ready ? Theme.safe : Theme.textSecondary
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label { text: qsTr("Managed Prowlarr"); color: Theme.textPrimary; font.weight: Font.DemiBold }
                    Label { text: root.prowlarrManager.stateLabel; color: Theme.textSecondary }
                }
                Button {
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
            color: "#ff9b9b"
            wrapMode: Text.WordWrap
        }

        Label {
            text: qsTr("Movie metadata")
            color: Theme.textPrimary
            font.pixelSize: Theme.titleSize
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
            TextField {
                id: tmdbToken
                Layout.fillWidth: true
                placeholderText: root.providerManager.hasTmdbToken
                                 ? qsTr("TMDB token saved")
                                 : qsTr("TMDB API Read Access Token")
                echoMode: TextInput.Password
            }
            Button {
                text: qsTr("Save token")
                onClicked: {
                    if (root.providerManager.saveTmdbToken(tmdbToken.text))
                        tmdbToken.clear()
                }
            }
            Button {
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
            TextField {
                id: providerName
                Layout.preferredWidth: 190
                placeholderText: qsTr("Provider name")
            }
            ComboBox {
                id: providerKind
                model: ["Torznab", "Prowlarr"]
            }
            TextField {
                id: providerEndpoint
                Layout.fillWidth: true
                placeholderText: qsTr("https://example.test/api")
            }
            TextField {
                id: providerApiKey
                Layout.preferredWidth: 190
                placeholderText: qsTr("API key (optional)")
                echoMode: TextInput.Password
            }
            Button {
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
            color: "#ff9b9b"
            wrapMode: Text.WordWrap
        }

        ListView {
            Layout.fillWidth: true
            implicitHeight: Math.min(contentHeight, 150)
            model: root.providerManager.model
            spacing: 6
            clip: true
            delegate: Rectangle {
                id: providerDelegate
                required property int index
                required property string name
                required property string kind
                required property string endpoint
                width: ListView.view.width
                height: 52
                radius: Theme.radius
                color: Theme.raised
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    Label { text: providerDelegate.name; color: Theme.textPrimary; font.weight: Font.DemiBold }
                    Label { text: providerDelegate.kind; color: Theme.textSecondary }
                    Label { Layout.fillWidth: true; text: providerDelegate.endpoint; color: Theme.textSecondary; elide: Text.ElideMiddle }
                    ToolButton {
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
            font.pixelSize: Theme.titleSize
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
            TextField {
                id: subtitleLanguages
                Layout.preferredWidth: 220
                text: root.subtitleManager.preferredLanguages
                placeholderText: qsTr("nl,en")
                Accessible.name: qsTr("Preferred subtitle language codes")
                onAccepted: root.subtitleManager.setPreferredLanguages(text)
            }
            Button {
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
            TextField {
                id: openSubtitlesApiKey
                Layout.fillWidth: true
                placeholderText: root.subtitleManager.configured
                                 ? qsTr("OpenSubtitles API key saved")
                                 : qsTr("API key")
                echoMode: TextInput.Password
            }
            TextField {
                id: openSubtitlesUsername
                Layout.preferredWidth: 210
                placeholderText: root.subtitleManager.username.length > 0
                                 ? root.subtitleManager.username : qsTr("Username")
            }
            TextField {
                id: openSubtitlesPassword
                Layout.preferredWidth: 210
                placeholderText: qsTr("Password")
                echoMode: TextInput.Password
            }
            Button {
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
            Button {
                text: qsTr("Remove")
                visible: root.subtitleManager.configured
                onClicked: root.subtitleManager.clearCredentials()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.subtitleManager.errorMessage.length > 0
            text: root.subtitleManager.errorMessage
            color: "#ff9b9b"
            wrapMode: Text.WordWrap
        }

        Item { implicitHeight: 1 }
        }
    }
}
