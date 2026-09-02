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
        property var results: [
            { language: "nl", release: "Test.NL", downloads: 12,
              trusted: true, hearingImpaired: false },
            { language: "en", release: "Test.EN", downloads: 34,
              trusted: false, hearingImpaired: false }
        ]
        property int searchCalls: 0
        property int cancelCalls: 0
        property int downloadRow: -1
        function search(query, languages) { searchCalls += 1 }
        function download(row) { downloadRow = row }
        function cancel() { cancelCalls += 1 }
    }

    ApplicationWindow {
        id: subtitleWindow
        width: 900
        height: 650
        visible: true
        SubtitleSearchDialog {
            id: dialog
            manager: fakeManager
            query: "Test movie"
        }
    }

    function init() {
        dialog.close()
        tryCompare(dialog, "opened", false)
        fakeManager.searchCalls = 0
        fakeManager.cancelCalls = 0
    }

    function test_openStartsProtectedSearch() {
        subtitleWindow.requestActivate()
        tryCompare(subtitleWindow, "active", true)
        dialog.open()
        tryCompare(fakeManager, "searchCalls", 1)
        tryCompare(findChild(dialog, "subtitleSearchField"), "activeFocus", true)
        dialog.close()
        tryCompare(fakeManager, "cancelCalls", 1)
    }

    function test_controller_navigation_reaches_results_and_close() {
        subtitleWindow.requestActivate()
        tryCompare(subtitleWindow, "active", true)
        fakeManager.downloadRow = -1
        dialog.open()
        tryCompare(findChild(dialog, "subtitleSearchField"), "activeFocus", true)
        tryVerify(function() { return dialog.controllerResultButtons.length === 2 })

        verify(dialog.handleControllerNavigation(1, 0))
        tryCompare(findChild(dialog, "subtitleSearchButton"), "activeFocus", true)
        verify(dialog.handleControllerNavigation(0, 1))
        const first = dialog.controllerResultButtons.find(
                    button => button.resultIndex === 0)
        verify(first !== undefined)
        tryCompare(first, "activeFocus", true)
        compare(first.focusBorderColor, Theme.textPrimary)
        verify(dialog.handleControllerNavigation(0, 1))
        const second = dialog.controllerResultButtons.find(
                    button => button.resultIndex === 1)
        verify(second !== undefined)
        tryCompare(second, "activeFocus", true)
        second.controllerActivate()
        compare(fakeManager.downloadRow, 1)
        verify(dialog.handleControllerNavigation(0, 1))
        tryCompare(findChild(dialog, "subtitleSearchCloseButton"), "activeFocus", true)
        dialog.close()
    }
}
