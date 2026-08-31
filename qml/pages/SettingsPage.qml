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

    FileDialog {
        id: profileDialog
        title: qsTr("Choose an OpenVPN profile")
        nameFilters: [qsTr("OpenVPN profiles (*.ovpn)")]
        onAccepted: root.vpnManager.importProfile(selectedFile)
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

                Rectangle {
                    implicitWidth: 9
                    implicitHeight: 9
                    radius: 5
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

        Item { implicitHeight: 1 }
        }
    }
}
