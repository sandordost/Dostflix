pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property var movieModel
    required property var prowlarrManager
    required property var torrentEngine
    property string pendingTitle: ""
    property string pendingMagnet: ""
    property string pendingDownload: ""
    property string pendingPoster: ""

    Dialog {
        id: releaseDialog
        anchors.centerIn: parent
        width: Math.min(540, root.width - 48)
        height: 220
        modal: true
        title: qsTr("Start this release?")
        background: Rectangle { radius: Theme.radiusLarge; color: Theme.panel }

        footer: DialogButtonBox {
            background: Item {}
            AppButton {
                text: qsTr("Cancel")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                onClicked: releaseDialog.reject()
            }
            AppButton {
                text: qsTr("Start release")
                primary: true
                enabled: root.pendingMagnet.length > 0 || root.pendingDownload.length > 0
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: {
                    releaseDialog.accept()
                    root.prowlarrManager.prepareRelease(
                                root.pendingTitle, root.pendingMagnet, root.pendingDownload)
                }
            }
        }

        contentItem: Label {
            text: root.pendingMagnet.length > 0 || root.pendingDownload.length > 0
                  ? qsTr("Dostflix will download and retain “%1”. Torrent traffic remains blocked whenever VPN protection is unavailable.").arg(root.pendingTitle)
                  : qsTr("This release has no usable download link.")
            color: Theme.textPrimary
            wrapMode: Text.WordWrap
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        Label {
            text: qsTr("Results")
            color: Theme.textPrimary
            font.pixelSize: Theme.headingSize
            font.weight: Font.DemiBold
        }
        RowLayout {
            Layout.fillWidth: true
            visible: root.prowlarrManager.searchBusy || root.prowlarrManager.searchError.length > 0
            spacing: 8
            BusyIndicator {
                implicitWidth: 22
                implicitHeight: 22
                running: root.prowlarrManager.searchBusy
                visible: running
            }
            Label {
                Layout.fillWidth: true
                text: root.prowlarrManager.searchBusy
                      ? qsTr("Searching all configured indexers…")
                      : root.prowlarrManager.searchError
                color: root.prowlarrManager.searchError.length > 0 ? Theme.danger : Theme.textSecondary
                wrapMode: Text.WordWrap
            }
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: torrentStatus.implicitHeight + 24
            radius: Theme.radiusLarge
            color: Theme.surface
            visible: root.torrentEngine.active
                     || root.torrentEngine.errorMessage.length > 0
                     || root.prowlarrManager.releaseBusy
                     || root.prowlarrManager.releaseError.length > 0

            ColumnLayout {
                id: torrentStatus
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    BusyIndicator {
                        implicitWidth: 22
                        implicitHeight: 22
                        running: root.prowlarrManager.releaseBusy
                                 || (root.torrentEngine.active && !root.torrentEngine.bufferReady)
                    }
                    Label {
                        Layout.fillWidth: true
                        text: root.prowlarrManager.releaseBusy
                              ? qsTr("Retrieving torrent file…")
                              : (root.torrentEngine.title.length > 0
                                 ? root.torrentEngine.title + " — " + root.torrentEngine.stateLabel
                                 : root.torrentEngine.stateLabel)
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                    }
                    AppButton {
                        text: qsTr("Cancel")
                        visible: root.torrentEngine.active
                        onClicked: root.torrentEngine.cancel()
                    }
                }

                ProgressBar {
                    Layout.fillWidth: true
                    visible: root.torrentEngine.active
                    from: 0
                    to: 1
                    value: root.torrentEngine.progress
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.torrentEngine.active
                    text: root.torrentEngine.downloadRate < 104857
                          ? qsTr("%1 KiB/s · %2 peers (%3 seeds) · %4 seconds buffered")
                              .arg((root.torrentEngine.downloadRate / 1024).toFixed(1))
                              .arg(root.torrentEngine.peerCount)
                              .arg(root.torrentEngine.seedCount)
                              .arg(Math.min(root.torrentEngine.bufferSeconds, 999).toFixed(0))
                          : qsTr("%1 MiB/s · %2 peers (%3 seeds) · %4 seconds buffered")
                              .arg((root.torrentEngine.downloadRate / 1048576).toFixed(1))
                              .arg(root.torrentEngine.peerCount)
                              .arg(root.torrentEngine.seedCount)
                              .arg(Math.min(root.torrentEngine.bufferSeconds, 999).toFixed(0))
                    color: Theme.textSecondary
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.torrentEngine.errorMessage.length > 0
                    text: root.torrentEngine.errorMessage
                    color: Theme.danger
                    wrapMode: Text.WordWrap
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.prowlarrManager.releaseError.length > 0
                    text: root.prowlarrManager.releaseError
                    color: Theme.danger
                    wrapMode: Text.WordWrap
                }

                Label {
                    visible: root.torrentEngine.needsFileSelection
                    text: qsTr("Choose the video to play")
                    color: Theme.textPrimary
                    font.weight: Font.DemiBold
                }

                Repeater {
                    model: root.torrentEngine.needsFileSelection
                           ? root.torrentEngine.videoFiles : null
                    delegate: Button {
                        required property int index
                        required property string path
                        required property double sizeBytes
                        Layout.fillWidth: true
                        text: path + " · " + (sizeBytes / 1073741824).toFixed(2) + " GiB"
                        onClicked: root.torrentEngine.selectVideoFile(index)
                    }
                }
            }
        }
        MovieGrid {
            Layout.fillWidth: true
            Layout.fillHeight: true
            movieModel: root.movieModel
            onReleaseSelected: (title, magnetUrl, downloadUrl, posterUrl) => {
                root.pendingTitle = title
                root.pendingMagnet = magnetUrl
                root.pendingDownload = downloadUrl
                root.pendingPoster = posterUrl
                releaseDialog.open()
            }
        }
    }
}
