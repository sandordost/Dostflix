import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "LibraryPage"
    when: windowShown

    ListModel {
        id: movies
        ListElement { title: "Local movie"; posterUrl: "" }
    }
    QtObject {
        id: fakeLibrary
        property var model: movies
        property int count: movies.count
        property string directory: "/tmp/Movies"
        property string errorMessage: ""
        property int playCalls: 0
        function refresh() {}
        function play(row) { playCalls += 1 }
    }
    ApplicationWindow {
        width: 700
        height: 500
        visible: true
        LibraryPage {
            id: page
            anchors.fill: parent
            libraryManager: fakeLibrary
        }
    }

    function test_localMovieCanBePlayed() {
        const button = findChild(page, "libraryPlayButton")
        verify(button !== null)
        mouseClick(button)
        compare(fakeLibrary.playCalls, 1)
    }
}
