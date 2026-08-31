import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

ApplicationWindow {
    id: window
    required property var appController
    required property var movieModel
    width: 1280
    height: 760
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: qsTr("Dostflix")
    color: Theme.canvas
    property int pageIndex: 0

    Image {
        anchors.fill: parent
        source: "qrc:/qt/qml/Dostflix/assets/backgrounds/dust-background.jpg"
        fillMode: Image.PreserveAspectCrop
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        AppHeader {
            Layout.fillWidth: true
            vpnLabel: qsTr("VPN not configured")
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            SidePanel {
                Layout.preferredWidth: 255
                Layout.fillHeight: true
                currentIndex: window.pageIndex
                onPageRequested: index => window.pageIndex = index
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: Theme.radius
                color: Qt.rgba(0.071, 0.071, 0.078, Theme.panelOpacity)

                StackLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    currentIndex: window.pageIndex
                    DiscoverPage { movieModel: window.movieModel }
                    LibraryPage {}
                    DownloadsPage {}
                    SettingsPage {}
                }
            }
        }
    }

    NowWatchingCard {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 24
        controller: window.appController
        onReturnRequested: window.pageIndex = 0
    }
}
