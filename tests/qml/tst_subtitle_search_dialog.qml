import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "SubtitleSearchDialog"
    when: windowShown

    QtObject {
        id: fakeManager
        property bool configured: true
        property bool networkReady: true
        property bool busy: false
        property string statusLabel: ""
        property string errorMessage: ""
        property var results: []
        property int searchCalls: 0
        property int cancelCalls: 0
        function search(query, languages) { searchCalls += 1 }
        function download(row) {}
        function cancel() { cancelCalls += 1 }
    }

    ApplicationWindow {
        width: 900
        height: 650
        visible: true
        SubtitleSearchDialog {
            id: dialog
            manager: fakeManager
            query: "Test movie"
        }
    }

    function test_openStartsProtectedSearch() {
        dialog.open()
        tryCompare(fakeManager, "searchCalls", 1)
        dialog.close()
        tryCompare(fakeManager, "cancelCalls", 1)
    }
}
