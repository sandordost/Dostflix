pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property var libraryManager
    required property var metadataManager

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: qsTr("Library")
                color: Theme.textPrimary
                font.pixelSize: Theme.titleSize
                font.weight: Font.Bold
            }
            Label {
                text: qsTr("%1 movies").arg(root.libraryManager.count)
                color: Theme.textSecondary
            }
            ToolButton {
                icon.name: "view-refresh-symbolic"
                Accessible.name: qsTr("Rescan movie library")
                onClicked: root.libraryManager.refresh()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.metadataManager.busy || root.metadataManager.errorMessage.length > 0
            BusyIndicator {
                implicitWidth: 22
                implicitHeight: 22
                running: root.metadataManager.busy
                visible: running
            }
            Label {
                Layout.fillWidth: true
                text: root.metadataManager.busy
                      ? root.metadataManager.stateLabel : root.metadataManager.errorMessage
                color: root.metadataManager.errorMessage.length > 0 ? "#ff9b9b" : Theme.textSecondary
                elide: Text.ElideRight
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillHeight: true
            visible: root.libraryManager.count === 0
            text: qsTr("Your library is empty\nAdd video files to %1").arg(root.libraryManager.directory)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: Theme.textSecondary
            font.pixelSize: Theme.bodySize
        }

        GridView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.libraryManager.count > 0
            clip: true
            cellWidth: 190
            cellHeight: 355
            model: root.libraryManager.model
            delegate: Rectangle {
                id: movieDelegate
                required property int index
                required property string title
                required property url posterUrl
                required property int year
                required property int durationSeconds
                required property string synopsis
                width: 174
                height: 339
                radius: Theme.radius
                color: Theme.raised

                Image {
                    id: poster
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 8
                    height: width / Theme.posterAspectRatio
                    source: movieDelegate.posterUrl.toString().length > 0
                            ? movieDelegate.posterUrl
                            : "qrc:/qt/qml/Dostflix/assets/images/poster-placeholder.svg"
                    fillMode: Image.PreserveAspectCrop
                }
                Label {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: poster.bottom
                    anchors.margins: 8
                    text: movieDelegate.title
                    color: Theme.textPrimary
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                Label {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: poster.bottom
                    anchors.topMargin: 34
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    text: (movieDelegate.year > 0 ? movieDelegate.year : qsTr("Unknown year"))
                          + (movieDelegate.durationSeconds > 0
                             ? qsTr(" · %1 min").arg(Math.round(movieDelegate.durationSeconds / 60)) : "")
                    color: Theme.textSecondary
                    font.pixelSize: Math.max(11, Theme.bodySize - 2)
                    elide: Text.ElideRight
                }
                ToolTip.visible: movieMouse.containsMouse && movieDelegate.synopsis.length > 0
                ToolTip.text: movieDelegate.synopsis
                MouseArea {
                    id: movieMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                }
                Button {
                    objectName: "libraryPlayButton"
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 8
                    text: qsTr("Play")
                    icon.name: "media-playback-start-symbolic"
                    onClicked: root.libraryManager.play(movieDelegate.index)
                }
            }
        }
    }
}
