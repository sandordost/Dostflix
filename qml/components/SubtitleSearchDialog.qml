pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Dialog {
    id: root
    required property var manager
    property string query: ""
    signal settingsRequested()
    anchors.centerIn: parent
    width: Math.min(760, parent ? parent.width - 80 : 760)
    height: Math.min(580, parent ? parent.height - 80 : 580)
    modal: true
    title: qsTr("Find subtitles")
    standardButtons: Dialog.Close

    onOpened: {
        if (manager.configured && manager.networkReady)
            manager.search(query, "nl,en")
    }
    onClosed: manager.cancel()

    contentItem: ColumnLayout {
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: searchField
                Layout.fillWidth: true
                text: root.query
                placeholderText: qsTr("Movie or release title")
                onAccepted: root.manager.search(text, "nl,en")
            }
            Button {
                text: qsTr("Search")
                enabled: !root.manager.busy && root.manager.configured && root.manager.networkReady
                onClicked: root.manager.search(searchField.text, "nl,en")
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !root.manager.configured
            text: qsTr("Configure your OpenSubtitles account in Settings first.")
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }
        Button {
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
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            model: root.manager.results
            delegate: Rectangle {
                id: resultDelegate
                required property int index
                required property var modelData
                width: ListView.view.width
                height: 70
                radius: Theme.radius
                color: Theme.raised
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10
                    Label {
                        text: resultDelegate.modelData.language.toUpperCase()
                        color: Theme.textPrimary
                        font.weight: Font.Bold
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
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
                    Button {
                        text: qsTr("Download")
                        enabled: !root.manager.busy
                        onClicked: root.manager.download(resultDelegate.index)
                    }
                }
            }
        }
    }
}
