import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "DownloadsPage"
    when: windowShown

    QtObject {
        id: fakeDownload
        property bool active: false
        property bool hasPending: true
        property bool hasTransfer: true
        property bool playable: true
        property string title: "Saved movie"
        property double bytesWritten: 25
        property double expectedSize: 100
        property double bytesRemaining: 75
        property double availableBytes: 1000000
        property bool diskSpaceReady: true
        property string partialFileName: "movie.mkv.dostflix.part"
        property real progress: 0.25
        property string stateLabel: "Download paused"
        property string errorMessage: ""
        property int resumeCalls: 0
        property int playCalls: 0
        property int removeCalls: 0
        function pause() {}
        function resume() { resumeCalls += 1 }
        function play() { playCalls += 1 }
        function remove() { removeCalls += 1 }
    }
    ApplicationWindow {
        width: 700
        height: 500
        visible: true
        DownloadsPage {
            id: page
            anchors.fill: parent
            downloadManager: fakeDownload
        }
    }

    function test_pausedDownloadCanResume() {
        const button = findChild(page, "resumeDownloadButton")
        verify(button !== null)
        mouseClick(button)
        compare(fakeDownload.resumeCalls, 1)
    }

    function test_incompleteFileIsIdentified() {
        const label = findChild(page, "incompleteFileLabel")
        verify(label !== null)
        verify(label.text.includes("movie.mkv.dostflix.part"))
        verify(label.visible)
    }

    function test_savedDownloadCanPlay() {
        const button = findChild(page, "playDownloadButton")
        verify(button !== null)
        mouseClick(button)
        tryCompare(fakeDownload, "playCalls", 1)
    }

    function test_savedDownloadCanBeRemovedAfterConfirmation() {
        const button = findChild(page, "removeDownloadButton")
        verify(button !== null)
        mouseClick(button)
        const dialog = findChild(page, "removeDownloadDialog")
        verify(dialog !== null)
        tryCompare(dialog, "visible", true)
        const confirm = findChild(dialog, "confirmRemoveDownloadButton")
        verify(confirm !== null)
        tryCompare(confirm, "visible", true)
        mouseClick(confirm)
        tryCompare(fakeDownload, "removeCalls", 1)
    }
}
