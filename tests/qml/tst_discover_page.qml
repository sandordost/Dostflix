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
        property bool active: false
        property bool bufferReady: false
        property string errorMessage: ""
        property string stateLabel: "Building playback buffer…"
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
        while (releases.count < 24) {
            const index = releases.count
            releases.append({
                title: "Movie " + index, year: 2026, quality: "1080p",
                seederCount: 42, posterUrl: "", sourceLabel: "Indexer",
                magnetUrl: "magnet:?xt=test" + index, downloadUrl: ""
            })
        }
        page.listMode = false
        page.selectedReleaseKey = ""
        fakeProwlarr.prepareCalls = 0
        fakeProwlarr.releaseBusy = false
        fakeProwlarr.releaseError = ""
        fakeTorrent.active = false
        fakeTorrent.errorMessage = ""
        page.ensureResultVisible(0)
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

    function test_selected_thumbnail_owns_transfer_status() {
        page.startRelease("Movie", "magnet:?xt=test", "")
        fakeProwlarr.releaseBusy = true
        compare(page.transferLoading, true)
        compare(page.transferStatusText, "Retrieving torrent…")
        verify(page.selectedReleaseKey.length > 0)
    }

    function test_grid_and_list_selection_scroll_into_view() {
        const grid = findChild(page, "movieGrid")
        const list = findChild(page, "movieList")
        verify(grid !== null)
        verify(list !== null)
        tryVerify(function() { return grid.contentHeight > grid.height })
        verify(page.ensureResultVisible(releases.count - 1))
        tryVerify(function() { return grid.contentY > 0 })

        page.listMode = true
        tryVerify(function() { return list.visible })
        tryVerify(function() { return list.contentHeight > list.height })
        verify(page.ensureResultVisible(releases.count - 1))
        tryVerify(function() { return list.contentY > 0 })
    }
}
