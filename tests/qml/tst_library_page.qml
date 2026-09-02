import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "LibraryPage"
    when: windowShown

    ListModel {
        id: movies
        ListElement {
            title: "Local movie"; posterUrl: ""; year: 1999
            durationSeconds: 8160; watchedSeconds: 0
            synopsis: "A local movie synopsis."
        }
    }
    QtObject {
        id: fakeMetadata
        property bool busy: false
        property string stateLabel: ""
        property string errorMessage: ""
        function refresh() {}
    }
    QtObject {
        id: fakeLibrary
        property var model: movies
        property int count: movies.count
        property string directory: "/tmp/Movies"
        property string errorMessage: ""
        property int playCalls: 0
        property bool lastRestart: false
        function refresh() {}
        function play(row, restart) { playCalls += 1; lastRestart = restart }
        function clearPlaybackSession() {}
    }
    ApplicationWindow {
        id: libraryWindow
        width: 700
        height: 500
        visible: true
        LibraryPage {
            id: page
            anchors.fill: parent
            libraryManager: fakeLibrary
            metadataManager: fakeMetadata
        }
    }

    function init() {
        libraryWindow.requestActivate()
        tryCompare(libraryWindow, "active", true)
        while (movies.count < 12) {
            const index = movies.count
            movies.append({
                title: "Local movie " + index, posterUrl: "", year: 2000 + index,
                durationSeconds: 7200, watchedSeconds: 0,
                synopsis: "A local movie synopsis."
            })
        }
        movies.setProperty(0, "watchedSeconds", 0)
        fakeLibrary.playCalls = 0
        fakeLibrary.lastRestart = false
        page.ensureMovieVisible(0)
    }

    function test_localMovieCanBePlayed() {
        const list = findChild(page, "libraryList")
        verify(list !== null)
        tryVerify(function() { return list.contentItem.children.length > 0 })
        compare(list.contentItem.children[0].height, 132)
        const button = findChild(page, "libraryPlayButton")
        verify(button !== null)
        mouseClick(button)
        compare(fakeLibrary.playCalls, 1)
        verify(fakeLibrary.lastRestart)
    }

    function test_continue_resumes_without_a_dialog() {
        movies.setProperty(0, "watchedSeconds", 125)
        const button = findChild(page, "libraryPlayButton")
        mouseClick(button)
        compare(fakeLibrary.playCalls, 1)
        verify(!fakeLibrary.lastRestart)
        verify(findChild(page, "resumePlaybackDialog") === null)
    }

    function test_refresh_is_the_top_controller_target() {
        verify(page.focusFirstControl())
        const refresh = findChild(page, "libraryRefreshButton")
        verify(refresh !== null)
        compare(refresh.focus, true)
    }

    function test_controller_selection_scrolls_into_view() {
        const list = findChild(page, "libraryList")
        verify(list !== null)
        tryVerify(function() { return list.contentHeight > list.height })
        verify(page.ensureMovieVisible(movies.count - 1))
        tryVerify(function() { return list.contentY > 0 })
    }

    function test_list_controller_navigation_stays_on_rows() {
        const list = findChild(page, "libraryList")
        verify(page.focusMovie(0))
        tryCompare(list, "currentIndex", 0)
        tryCompare(list.currentItem, "activeFocus", true)
        verify(page.handleControllerNavigation(-1, 0))
        compare(list.currentIndex, 0)
        compare(list.currentItem.activeFocus, true)
        verify(page.handleControllerNavigation(0, 1))
        tryCompare(list, "currentIndex", 1)
        verify(page.handleControllerNavigation(0, -1))
        tryCompare(list, "currentIndex", 0)
        verify(page.handleControllerNavigation(0, -1))
        tryCompare(findChild(page, "libraryRefreshButton"), "activeFocus", true)
    }
}
