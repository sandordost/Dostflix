import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "AppHeader"
    when: windowShown

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
        }
    }

    function test_transfer_progress_is_not_in_header() {
        const status = findChild(header, "headerTransferStatus")
        const progress = findChild(header, "headerTransferProgress")
        verify(status === null)
        verify(progress === null)
    }
}
