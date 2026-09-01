pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property var downloadManager

    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        Label {
            text: qsTr("Downloads")
            color: Theme.textPrimary
            font.pixelSize: Theme.titleSize
            font.weight: Font.Bold
        }
        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !root.downloadManager.hasPending
            text: qsTr("No active or resumable downloads")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: Theme.textSecondary
            font.pixelSize: Theme.bodySize
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: transferContent.implicitHeight + 28
            visible: root.downloadManager.hasPending
            radius: Theme.radius
            color: Theme.raised

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
                        visible: !root.downloadManager.active
                        onClicked: root.downloadManager.resume()
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
                    Layout.fillWidth: true
                    visible: root.downloadManager.errorMessage.length > 0
                    text: root.downloadManager.errorMessage
                    color: "#ff9b9b"
                    wrapMode: Text.WordWrap
                }
            }
        }
        Item {
            Layout.fillHeight: true
            visible: root.downloadManager.hasPending
        }
    }
}
