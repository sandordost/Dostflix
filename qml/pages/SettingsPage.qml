import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property var vpnManager

    FileDialog {
        id: profileDialog
        title: qsTr("Choose an OpenVPN profile")
        nameFilters: [qsTr("OpenVPN profiles (*.ovpn)")]
        onAccepted: root.vpnManager.importProfile(selectedFile)
    }

    ColumnLayout {
        anchors.fill: parent
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

        Item { Layout.fillHeight: true }
    }
}
