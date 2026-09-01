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
    width: Math.min(Theme.px(760), parent ? parent.width - Theme.px(80) : Theme.px(760))
    height: Math.min(Theme.px(580), parent ? parent.height - Theme.px(80) : Theme.px(580))
    modal: true
    title: qsTr("Find subtitles")
    background: Rectangle { radius: Theme.radiusLarge; color: Theme.panel }
    footer: DialogButtonBox {
        background: Item {}
        AppButton {
            text: qsTr("Close")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: root.reject()
        }
    }

    onOpened: {
        if (manager.configured && manager.networkReady)
            manager.search(query)
    }
    onClosed: manager.cancel()

    contentItem: ColumnLayout {
        spacing: Theme.px(12)

        RowLayout {
            Layout.fillWidth: true
            AppTextField {
                id: searchField
                Layout.fillWidth: true
                text: root.query
                placeholderText: qsTr("Movie or release title")
                onAccepted: root.manager.search(text)
            }
            AppButton {
                text: qsTr("Search")
                primary: true
                enabled: !root.manager.busy && root.manager.configured && root.manager.networkReady
                onClicked: root.manager.search(searchField.text)
            }
        }

        Label {
            Layout.fillWidth: true
            visible: !root.manager.configured
            text: qsTr("Configure your OpenSubtitles account in Settings first.")
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
        }
        AppButton {
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
            spacing: Theme.px(6)
            model: root.manager.results
            delegate: Rectangle {
                id: resultDelegate
                required property int index
                required property var modelData
                width: ListView.view.width
                height: Theme.px(70)
                radius: Theme.radius
                color: Theme.raised
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.px(10)
                    spacing: Theme.px(10)
                    Label {
                        text: resultDelegate.modelData.language.toUpperCase()
                        color: Theme.textPrimary
                        font.weight: Font.Bold
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.px(2)
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
                    AppButton {
                        text: qsTr("Download")
                        primary: true
                        enabled: !root.manager.busy
                        onClicked: root.manager.download(resultDelegate.index)
                    }
                }
            }
        }
    }
}
