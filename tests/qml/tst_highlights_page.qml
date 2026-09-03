import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "HighlightsPage"
    when: windowShown

    ListModel { id: trending; ListElement { title: "One"; year: 2026; rating: 8.1; posterUrl: "" } ListElement { title: "Two"; year: 2026; rating: 8.2; posterUrl: "" } }
    ListModel { id: best; ListElement { title: "Best"; year: 2026; rating: 9.0; posterUrl: "" } }
    ListModel { id: rated; ListElement { title: "Classic"; year: 1972; rating: 8.8; posterUrl: "" } }
    QtObject {
        id: fakeManager
        property var trendingModel: trending
        property var bestOfYearModel: best
        property var highRatingsModel: rated
        property bool busy: false
        property bool configured: true
        property string errorMessage: ""
        property int bestOfYear: 2026
    }
    ApplicationWindow {
        id: highlightWindow
        width: 800; height: 620; visible: true
        HighlightsPage { id: page; anchors.fill: parent; manager: fakeManager }
    }
    SignalSpy { id: selectedSpy; target: page; signalName: "movieSelected" }

    function init() {
        highlightWindow.requestActivate()
        tryCompare(highlightWindow, "active", true)
        selectedSpy.clear()
        page.activeShelf = 0
    }

    function test_controller_moves_within_rows_and_resets_column_between_rows() {
        verify(page.focusFirstResult())
        tryVerify(function() { return page.ownsActiveFocus() })
        compare(page.activeColumn, 0)
        verify(page.handleControllerNavigation(1, 0))
        compare(page.activeColumn, 1)
        verify(page.handleControllerNavigation(0, 1))
        compare(page.activeShelf, 1)
        compare(page.activeColumn, 0)
    }

    function test_card_activation_returns_title_for_search() {
        verify(page.focusFirstResult())
        verify(page.activateCurrent())
        compare(selectedSpy.count, 1)
        compare(selectedSpy.signalArguments[0][0], "One")
    }
}
