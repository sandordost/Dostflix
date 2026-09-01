import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "AppHeader"
    when: windowShown

    QtObject {
        id: fakeProwlarr
        property bool releaseBusy: false
        property string releaseError: ""
    }
    QtObject {
        id: fakeTorrent
        property bool active: true
        property bool bufferReady: false
        property string errorMessage: ""
        property string title: "Movie"
        property string stateLabel: "Buffering"
        property real progress: 0.35
        function cancel() {}
    }
    ApplicationWindow {
        width: 1000
        height: 120
        visible: true
        AppHeader {
            id: header
            anchors.fill: parent
            vpnLabel: "VPN connected"
            vpnConnected: true
            vpnBusy: false
            prowlarrManager: fakeProwlarr
            torrentEngine: fakeTorrent
        }
    }

    function test_transfer_progress_is_in_header() {
        const status = findChild(header, "headerTransferStatus")
        const progress = findChild(header, "headerTransferProgress")
        verify(status !== null)
        verify(progress !== null)
        verify(status.visible)
        compare(progress.value, 0.35)
    }
}
