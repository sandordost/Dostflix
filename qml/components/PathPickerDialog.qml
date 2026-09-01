pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.folderlistmodel
import Dostflix

Dialog {
    id: root
    property bool folderMode: false
    property url initialFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
    property var fileNameFilters: ["*"]
    property url selectedUrl: ""
    signal pathChosen(url path)

    modal: true
    width: Math.min(760, parent ? parent.width - 48 : 760)
    height: Math.min(620, parent ? parent.height - 48 : 620)
    padding: 0
    anchors.centerIn: parent
    background: Rectangle {
        radius: Theme.radiusLarge
        color: Theme.panel
    }

    function openAt(folderUrl) {
        selectedUrl = ""
        folderModel.folder = folderUrl && folderUrl.toString().length > 0
                ? folderUrl : StandardPaths.writableLocation(StandardPaths.HomeLocation)
        open()
    }

    function chooseCurrent() {
        const chosen = root.folderMode ? folderModel.folder : root.selectedUrl
        if (!chosen || chosen.toString().length === 0)
            return
        root.pathChosen(chosen)
        root.close()
    }

    FolderListModel {
        id: folderModel
        showDirs: true
        showDirsFirst: true
        showDotAndDotDot: false
        showFiles: !root.folderMode
        nameFilters: root.fileNameFilters
    }

    header: ColumnLayout {
        spacing: 10
        Item { Layout.preferredHeight: 4 }
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            text: root.title
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.headingSize
            font.weight: Font.DemiBold
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 18
            Layout.rightMargin: 18
            spacing: 8
            AppToolButton {
                icon.name: "go-up-symbolic"
                enabled: folderModel.parentFolder.toString().length > 0
                Accessible.name: qsTr("Parent folder")
                onClicked: folderModel.folder = folderModel.parentFolder
            }
            AppTextField {
                Layout.fillWidth: true
                readOnly: true
                text: decodeURIComponent(folderModel.folder.toString().replace("file://", ""))
                Accessible.name: qsTr("Current folder")
            }
            AppToolButton {
                icon.name: "go-home-symbolic"
                Accessible.name: qsTr("Home folder")
                onClicked: folderModel.folder = StandardPaths.writableLocation(StandardPaths.HomeLocation)
            }
        }
        Item { Layout.preferredHeight: 2 }
    }

    contentItem: Rectangle {
        color: Theme.canvas

        ListView {
            id: fileList
            anchors.fill: parent
            anchors.margins: 12
            clip: true
            spacing: 4
            model: folderModel
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                id: entry
                required property string fileName
                required property url fileUrl
                required property bool fileIsDir
                width: fileList.width
                height: 48
                radius: Theme.radiusSmall
                color: entry.fileUrl === root.selectedUrl
                       ? Theme.accentSoft
                       : (entryMouse.containsMouse ? Theme.raisedHover : "transparent")

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 12
                    AppIcon {
                        glyph: entry.fileIsDir ? "\uf07b" : "\uf15b"
                        color: entry.fileIsDir ? Theme.accent : Theme.textSecondary
                        font.pixelSize: Theme.iconSize
                        Layout.preferredWidth: Theme.iconSize
                        Layout.preferredHeight: Theme.iconSize
                    }
                    Label {
                        Layout.fillWidth: true
                        text: entry.fileName
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.bodySize
                        elide: Text.ElideMiddle
                    }
                }

                MouseArea {
                    id: entryMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (entry.fileIsDir)
                            root.selectedUrl = ""
                        else
                            root.selectedUrl = entry.fileUrl
                    }
                    onDoubleClicked: {
                        if (entry.fileIsDir) {
                            folderModel.folder = entry.fileUrl
                            root.selectedUrl = ""
                        } else {
                            root.selectedUrl = entry.fileUrl
                            root.chooseCurrent()
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }
    }

    footer: RowLayout {
        spacing: 8
        Item { Layout.preferredHeight: 10 }
        Label {
            Layout.leftMargin: 18
            Layout.fillWidth: true
            text: root.folderMode ? qsTr("Choose this folder")
                                  : (root.selectedUrl.toString().length > 0
                                     ? decodeURIComponent(root.selectedUrl.toString().split("/").pop())
                                     : qsTr("Select a file"))
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.captionSize
            elide: Text.ElideMiddle
        }
        AppButton {
            text: qsTr("Cancel")
            onClicked: root.close()
        }
        AppButton {
            Layout.rightMargin: 18
            text: root.folderMode ? qsTr("Choose folder") : qsTr("Open file")
            primary: true
            enabled: root.folderMode || root.selectedUrl.toString().length > 0
            onClicked: root.chooseCurrent()
        }
        Item { Layout.preferredHeight: 10 }
    }
}
