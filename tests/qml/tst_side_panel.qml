import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "SidePanelControllerSearch"
    when: windowShown

    QtObject {
        id: fakeControllerManager
        property bool connected: true
        property string searchButtonLabel: "Y"
        property string previousPageLabel: "LT"
        property string nextPageLabel: "RT"
    }

    ApplicationWindow {
        width: 500
        height: 600
        visible: true

        SidePanel {
            id: panel
            anchors.fill: parent
            currentIndex: 0
            searchEnabled: true
            compact: false
            controllerManager: fakeControllerManager
        }
    }

    SignalSpy {
        id: dismissedSpy
        target: panel
        signalName: "controllerSearchDismissed"
    }
    SignalSpy {
        id: searchSpy
        target: panel
        signalName: "searchRequested"
    }

    function init() {
        dismissedSpy.clear()
        const field = findChild(panel, "searchField")
        field.clear()
        searchSpy.clear()
        panel.controllerSearchActive = false
        panel.controllerSearchHadQuery = false
    }

    function test_search_only_submits_on_enter() {
        const field = findChild(panel, "searchField")
        field.text = "Arrival"
        wait(2100)
        compare(searchSpy.count, 0)
        field.controllerAccept()
        compare(searchSpy.count, 1)
        compare(searchSpy.signalArguments[0][0], "Arrival")
    }

    function test_search_is_only_in_controller_tab_chain_when_explicitly_opened() {
        const field = findChild(panel, "searchField")
        verify(field !== null)
        compare(field.activeFocusOnTab, false)
        compare(field.height, 46)

        panel.openControllerSearch()
        compare(panel.controllerSearchActive, true)
        compare(field.focus, true)

        verify(panel.closeControllerSearch())
        compare(dismissedSpy.count, 1)
        compare(dismissedSpy.signalArguments[0][0], false)
    }

    function test_back_does_not_report_unsubmitted_text_as_a_search() {
        const field = findChild(panel, "searchField")
        panel.openControllerSearch()
        field.text = "Sisu"
        compare(panel.controllerSearchHadQuery, false)

        verify(panel.closeControllerSearch())
        compare(dismissedSpy.count, 1)
        compare(dismissedSpy.signalArguments[0][0], false)
    }
}
