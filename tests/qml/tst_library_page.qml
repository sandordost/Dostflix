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
        movies.setProperty(0, "watchedSeconds", 0)
        fakeLibrary.playCalls = 0
        fakeLibrary.lastRestart = false
    }

    function test_localMovieCanBePlayed() {
        const button = findChild(page, "libraryPlayButton")
        verify(button !== null)
        mouseClick(button)
        compare(fakeLibrary.playCalls, 1)
        verify(fakeLibrary.lastRestart)
    }

    function test_resumeChoice() {
        movies.setProperty(0, "watchedSeconds", 125)
        const button = findChild(page, "libraryPlayButton")
        mouseClick(button)
        const dialog = findChild(page, "resumePlaybackDialog")
        tryCompare(dialog, "opened", true)
        const resume = findChild(page, "resumeMovieButton")
        mouseClick(resume)
        compare(fakeLibrary.playCalls, 1)
        verify(!fakeLibrary.lastRestart)
    }
}
