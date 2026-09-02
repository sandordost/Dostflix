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
    property bool listMode: false
    property string selectionError: ""
    property string selectedReleaseKey: ""

    readonly property bool transferLoading: prowlarrManager.releaseBusy
                                                   || (torrentEngine.active
                                                       && !torrentEngine.bufferReady)
    readonly property bool transferHasError: prowlarrManager.releaseError.length > 0
                                              || torrentEngine.errorMessage.length > 0
                                              || selectionError.length > 0
    readonly property string transferStatusText: {
        if (prowlarrManager.releaseBusy)
            return qsTr("Retrieving torrent…")
        if (torrentEngine.errorMessage.length > 0)
            return torrentEngine.errorMessage
        if (prowlarrManager.releaseError.length > 0)
            return prowlarrManager.releaseError
        if (selectionError.length > 0)
            return selectionError
        if (torrentEngine.active)
            return torrentEngine.stateLabel
        return ""
    }

    function releaseKey(title, magnetUrl, downloadUrl) {
        return title + "\n" + magnetUrl + "\n" + downloadUrl
    }

    function focusFirstResult() {
        return listMode ? movieList.focusFirstResult() : movieGrid.focusFirstResult()
    }

    function focusFirstControl() {
        return root.focusFirstResult()
    }

    function ensureResultVisible(index) {
        return listMode ? movieList.ensureIndexVisible(index)
                        : movieGrid.ensureIndexVisible(index)
    }

    function handleControllerNavigation(horizontal, vertical) {
        return listMode
                ? movieList.handleControllerNavigation(horizontal, vertical)
                : movieGrid.handleControllerNavigation(horizontal, vertical)
    }

    function startRelease(title, magnetUrl, downloadUrl) {
        selectionError = ""
        selectedReleaseKey = releaseKey(title, magnetUrl, downloadUrl)
        if (magnetUrl.length === 0 && downloadUrl.length === 0) {
            selectionError = qsTr("This release has no usable download link.")
            return
        }
        prowlarrManager.prepareRelease(title, magnetUrl, downloadUrl)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.px(12)

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: qsTr("Results")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.headingSize
                font.weight: Font.DemiBold
            }
            AppToolButton {
                objectName: "gridViewButton"
                icon.name: "view-grid-symbolic"
                primary: !root.listMode
                Accessible.name: qsTr("Grid view")
                ToolTip.visible: hovered
                ToolTip.text: Accessible.name
                onClicked: root.listMode = false
            }
            AppToolButton {
                objectName: "listViewButton"
                icon.name: "view-list-symbolic"
                primary: root.listMode
                Accessible.name: qsTr("List view")
                ToolTip.visible: hovered
                ToolTip.text: Accessible.name
                onClicked: root.listMode = true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.prowlarrManager.searchBusy
                     || root.prowlarrManager.searchError.length > 0
            spacing: Theme.px(8)
            BusyIndicator {
                implicitWidth: Theme.px(22)
                implicitHeight: Theme.px(22)
                running: root.prowlarrManager.searchBusy
                visible: running
            }
            Label {
                Layout.fillWidth: true
                text: root.prowlarrManager.searchBusy
                      ? qsTr("Searching all configured indexers…")
                      : root.prowlarrManager.searchError
                color: root.prowlarrManager.searchError.length > 0
                       ? Theme.danger : Theme.textSecondary
                wrapMode: Text.WordWrap
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: releaseDetails.implicitHeight + Theme.px(24)
            radius: Theme.radiusLarge
            color: Theme.surface
            visible: root.selectionError.length > 0
                     || root.torrentEngine.needsFileSelection

            ColumnLayout {
                id: releaseDetails
                anchors.fill: parent
                anchors.margins: Theme.px(12)
                spacing: Theme.px(8)

                Label {
                    Layout.fillWidth: true
                    visible: root.selectionError.length > 0
                    text: root.selectionError
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
                    delegate: AppButton {
                        required property int index
                        required property string path
                        required property double sizeBytes
                        Layout.fillWidth: true
                        alignLeft: true
                        text: path + " · " + (sizeBytes / 1073741824).toFixed(2) + " GiB"
                        onClicked: root.torrentEngine.selectVideoFile(index)
                    }
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.listMode ? 1 : 0

            MovieGrid {
                id: movieGrid
                movieModel: root.movieModel
                selectedReleaseKey: root.selectedReleaseKey
                transferLoading: root.transferLoading
                transferStatusText: root.transferStatusText
                transferHasError: root.transferHasError
                onReleaseSelected: (title, magnetUrl, downloadUrl, posterUrl) =>
                    root.startRelease(title, magnetUrl, downloadUrl)
            }
            MovieList {
                id: movieList
                movieModel: root.movieModel
                selectedReleaseKey: root.selectedReleaseKey
                transferLoading: root.transferLoading
                transferStatusText: root.transferStatusText
                transferHasError: root.transferHasError
                onReleaseSelected: (title, magnetUrl, downloadUrl, posterUrl) =>
                    root.startRelease(title, magnetUrl, downloadUrl)
            }
        }
    }
}
