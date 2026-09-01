pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property var downloadManager

    Dialog {
        id: removeDialog
        objectName: "removeDownloadDialog"
        anchors.centerIn: parent
        modal: true
        title: qsTr("Remove download?")
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: root.downloadManager.remove()
        contentItem: Label {
            width: 420
            text: qsTr("This removes the partial or completed movie file and its download history. This cannot be undone.")
            color: Theme.textPrimary
            wrapMode: Text.WordWrap
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        Label {
            text: qsTr("Downloads")
            color: Theme.textPrimary
            font.pixelSize: Theme.headingSize
            font.weight: Font.DemiBold
        }
        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !root.downloadManager.hasTransfer
            text: qsTr("No saved downloads")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: Theme.textSecondary
            font.pixelSize: Theme.bodySize
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: transferContent.implicitHeight + 28
            visible: root.downloadManager.hasTransfer
            radius: Theme.radiusLarge
            color: Theme.surface

            ColumnLayout {
                id: transferContent
                anchors.fill: parent
                anchors.margins: 14
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    BusyIndicator {
                        implicitWidth: 22
                        implicitHeight: 22
                        running: root.downloadManager.active
                        visible: running
                    }
                    Label {
                        Layout.fillWidth: true
                        text: root.downloadManager.title
                        color: Theme.textPrimary
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Button {
                        text: qsTr("Pause")
                        visible: root.downloadManager.active
                        onClicked: root.downloadManager.pause()
                    }
                    Button {
                        objectName: "resumeDownloadButton"
                        text: qsTr("Resume")
                        visible: root.downloadManager.hasPending
                                 && !root.downloadManager.active
                        onClicked: root.downloadManager.resume()
                    }
                    Button {
                        objectName: "playDownloadButton"
                        text: qsTr("Play")
                        icon.name: "media-playback-start-symbolic"
                        enabled: root.downloadManager.playable
                        onClicked: root.downloadManager.play()
                    }
                    Button {
                        objectName: "removeDownloadButton"
                        text: qsTr("Remove")
                        icon.name: "edit-delete-symbolic"
                        onClicked: removeDialog.open()
                    }
                }
                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: root.downloadManager.progress
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("%1 of %2 GiB · %3")
                          .arg((root.downloadManager.bytesWritten / 1073741824).toFixed(2))
                          .arg((root.downloadManager.expectedSize / 1073741824).toFixed(2))
                          .arg(root.downloadManager.stateLabel)
                    color: Theme.textSecondary
                    elide: Text.ElideRight
                }
                Label {
                    objectName: "incompleteFileLabel"
                    Layout.fillWidth: true
                    visible: root.downloadManager.hasPending
                    text: qsTr("Incomplete file: %1 · %2 GiB remaining · %3 GiB free")
                          .arg(root.downloadManager.partialFileName)
                          .arg((root.downloadManager.bytesRemaining / 1073741824).toFixed(2))
                          .arg((root.downloadManager.availableBytes / 1073741824).toFixed(2))
                    color: root.downloadManager.diskSpaceReady
                           ? Theme.textSecondary : Theme.danger
                    elide: Text.ElideMiddle
                }
                Label {
                    Layout.fillWidth: true
                    visible: root.downloadManager.errorMessage.length > 0
                    text: root.downloadManager.errorMessage
                    color: Theme.danger
                    wrapMode: Text.WordWrap
                }
            }
        }
        Item {
            Layout.fillHeight: true
            visible: root.downloadManager.hasTransfer
        }
    }
}
