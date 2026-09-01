import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "DiscoverPage"
    when: windowShown

    ListModel {
        id: releases
        ListElement {
            title: "Movie"; year: 2026; quality: "1080p"; seederCount: 42
            posterUrl: ""; sourceLabel: "Indexer"; magnetUrl: "magnet:?xt=test"
            downloadUrl: ""
        }
    }
    QtObject {
        id: fakeProwlarr
        property bool searchBusy: false
        property string searchError: ""
        property bool releaseBusy: false
        property string releaseError: ""
        property int prepareCalls: 0
        function prepareRelease(title, magnetUrl, downloadUrl) { prepareCalls += 1 }
    }
    QtObject {
        id: fakeTorrent
        property bool needsFileSelection: false
        property var videoFiles: []
        function selectVideoFile(index) {}
    }
    ApplicationWindow {
        width: 760
        height: 520
        visible: true
        DiscoverPage {
            id: page
            anchors.fill: parent
            movieModel: releases
            prowlarrManager: fakeProwlarr
            torrentEngine: fakeTorrent
        }
    }

    function init() {
        page.listMode = false
        fakeProwlarr.prepareCalls = 0
    }

    function test_view_toggle() {
        const listButton = findChild(page, "listViewButton")
        const gridButton = findChild(page, "gridViewButton")
        verify(listButton !== null)
        verify(gridButton !== null)
        mouseClick(listButton)
        verify(page.listMode)
        mouseClick(gridButton)
        verify(!page.listMode)
    }

    function test_release_starts_without_confirmation() {
        page.startRelease("Movie", "magnet:?xt=test", "")
        compare(fakeProwlarr.prepareCalls, 1)
    }
}
