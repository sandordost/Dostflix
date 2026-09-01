import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property string vpnLabel
    required property bool vpnConnected
    required property bool vpnBusy
    required property var prowlarrManager
    required property var torrentEngine
    readonly property bool transferVisible: prowlarrManager.releaseBusy
                                             || torrentEngine.active
                                             || torrentEngine.errorMessage.length > 0
                                             || prowlarrManager.releaseError.length > 0
    implicitHeight: Theme.headerHeight

    Image {
        id: logo
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: 52
        height: 52
        source: "qrc:/qt/qml/Dostflix/assets/icons/dostflix.svg"
        sourceSize: Qt.size(104, 104)
        Accessible.name: qsTr("Dostflix logo")
    }

    Label {
        id: brandTitle
        anchors.left: logo.right
        anchors.leftMargin: 14
        anchors.verticalCenter: logo.verticalCenter
        text: "Dostflix"
        color: Theme.textPrimary
        font.family: Theme.fontFamily
        font.pixelSize: 31
        font.weight: Font.DemiBold
    }

    Rectangle {
        id: vpnStatus
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: statusRow.implicitWidth + 24
        height: 38
        radius: 19
        color: Qt.rgba(0.08, 0.08, 0.09, 0.82)

        RowLayout {
            id: statusRow
            anchors.centerIn: parent
            spacing: 8

            BusyIndicator {
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                running: root.vpnBusy
                visible: running
            }
            Rectangle {
                Layout.preferredWidth: 8
                Layout.preferredHeight: 8
                radius: 4
                visible: !root.vpnBusy
                color: root.vpnConnected ? Theme.safe : Theme.textSecondary
            }
            Label {
                text: root.vpnLabel
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.captionSize
            }
        }
    }

    Rectangle {
        id: transferStatus
        objectName: "headerTransferStatus"
        anchors.left: brandTitle.right
        anchors.right: vpnStatus.left
        anchors.leftMargin: 36
        anchors.rightMargin: 36
        anchors.verticalCenter: parent.verticalCenter
        height: 58
        radius: Theme.radius
        color: Theme.surface
        visible: root.transferVisible && width >= 280

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                BusyIndicator {
                    Layout.preferredWidth: 18
                    Layout.preferredHeight: 18
                    running: root.prowlarrManager.releaseBusy
                             || (root.torrentEngine.active && !root.torrentEngine.bufferReady)
                    visible: running
                }
                Label {
                    Layout.fillWidth: true
                    text: root.prowlarrManager.releaseBusy
                          ? qsTr("Retrieving torrent…")
                          : (root.torrentEngine.errorMessage.length > 0
                             ? root.torrentEngine.errorMessage
                             : (root.prowlarrManager.releaseError.length > 0
                                ? root.prowlarrManager.releaseError
                                : root.torrentEngine.title + " — " + root.torrentEngine.stateLabel))
                    color: root.torrentEngine.errorMessage.length > 0
                           || root.prowlarrManager.releaseError.length > 0
                           ? Theme.danger : Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.captionSize
                    elide: Text.ElideRight
                }
                AppToolButton {
                    visible: root.torrentEngine.active
                    icon.name: "media-playback-stop-symbolic"
                    implicitWidth: 34
                    implicitHeight: 34
                    Accessible.name: qsTr("Cancel download")
                    onClicked: root.torrentEngine.cancel()
                }
            }

            ProgressBar {
                objectName: "headerTransferProgress"
                Layout.fillWidth: true
                Layout.preferredHeight: 4
                visible: root.torrentEngine.active
                from: 0
                to: 1
                value: root.torrentEngine.progress
            }
        }
    }
}
