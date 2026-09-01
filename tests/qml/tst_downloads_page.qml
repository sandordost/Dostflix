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
        property string title: "Saved movie"
        property double bytesWritten: 25
        property double expectedSize: 100
        property real progress: 0.25
        property string stateLabel: "Download paused"
        property string errorMessage: ""
        property int resumeCalls: 0
        function pause() {}
        function resume() { resumeCalls += 1 }
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
}
