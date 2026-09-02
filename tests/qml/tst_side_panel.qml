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
        property string previousPageLabel: "LB"
        property string nextPageLabel: "RB"
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

    function init() {
        dismissedSpy.clear()
        const field = findChild(panel, "searchField")
        field.clear()
        panel.controllerSearchActive = false
        panel.controllerSearchHadQuery = false
    }

    function test_search_is_only_in_controller_tab_chain_when_explicitly_opened() {
        const field = findChild(panel, "searchField")
        verify(field !== null)
        compare(field.activeFocusOnTab, false)

        panel.openControllerSearch()
        compare(panel.controllerSearchActive, true)
        compare(field.focus, true)

        verify(panel.closeControllerSearch())
        compare(dismissedSpy.count, 1)
        compare(dismissedSpy.signalArguments[0][0], false)
    }

    function test_back_reports_when_a_query_was_entered() {
        const field = findChild(panel, "searchField")
        panel.openControllerSearch()
        field.text = "Sisu"
        compare(panel.controllerSearchHadQuery, true)

        verify(panel.closeControllerSearch())
        compare(dismissedSpy.count, 1)
        compare(dismissedSpy.signalArguments[0][0], true)
    }
}
