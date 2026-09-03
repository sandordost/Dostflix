pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property var manager
    property int activeShelf: 0
    readonly property int activeColumn: shelves()[activeShelf].currentIndex
    signal movieSelected(string title)

    function shelves() { return [trendingShelf, bestShelf, highShelf] }

    function ownsActiveFocus() {
        let item = root.Window.window ? root.Window.window.activeFocusItem : null
        while (item) {
            if (item === root)
                return true
            item = item.parent
        }
        return false
    }

    function focusFirstResult() {
        const rows = shelves()
        for (let index = 0; index < rows.length; ++index) {
            if (rows[index].count > 0) {
                activeShelf = index
                ensureShelfVisible(rows[index])
                return rows[index].focusIndex(0)
            }
        }
        return false
    }

    function handleControllerNavigation(horizontal, vertical) {
        if (!ownsActiveFocus())
            return false
        const rows = shelves()
        if (vertical !== 0) {
            let next = activeShelf + (vertical > 0 ? 1 : -1)
            while (next >= 0 && next < rows.length && rows[next].count === 0)
                next += vertical > 0 ? 1 : -1
            if (next >= 0 && next < rows.length) {
                activeShelf = next
                ensureShelfVisible(rows[next])
                rows[next].focusIndex(0)
            }
            return true
        }
        return rows[activeShelf].handleHorizontal(horizontal)
    }

    function activateCurrent() {
        return shelves()[activeShelf].activateCurrent()
    }

    function ensureShelfVisible(shelf) {
        const position = shelf.mapToItem(content, 0, 0).y
        if (position < scroller.contentY)
            scroller.contentY = position
        else if (position + shelf.height > scroller.contentY + scroller.height)
            scroller.contentY = Math.min(scroller.contentHeight - scroller.height,
                                         position + shelf.height - scroller.height)
    }

    BusyIndicator {
        z: 2
        anchors.centerIn: parent
        running: root.manager.busy
        visible: running
    }

    Column {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.px(32), Theme.px(560))
        spacing: Theme.px(12)
        visible: !root.manager.configured
        Label {
            width: parent.width
            text: qsTr("Add a TMDB token in Settings to load movie highlights.")
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Label {
        z: 2
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        visible: root.manager.errorMessage.length > 0
        text: root.manager.errorMessage
        color: Theme.danger
        wrapMode: Text.WordWrap
    }

    Flickable {
        id: scroller
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width
        contentHeight: content.implicitHeight
        visible: root.manager.configured

        ColumnLayout {
            id: content
            width: scroller.width
            spacing: Theme.px(28)

            HighlightShelf {
                id: trendingShelf
                Layout.fillWidth: true
                title: qsTr("Trending")
                movieModel: root.manager.trendingModel
                onMovieSelected: title => root.movieSelected(title)
            }
            HighlightShelf {
                id: bestShelf
                Layout.fillWidth: true
                title: qsTr("Best of %1").arg(root.manager.bestOfYear)
                movieModel: root.manager.bestOfYearModel
                onMovieSelected: title => root.movieSelected(title)
            }
            HighlightShelf {
                id: highShelf
                Layout.fillWidth: true
                title: qsTr("High Ratings")
                movieModel: root.manager.highRatingsModel
                onMovieSelected: title => root.movieSelected(title)
            }
            Label {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Movie data from TMDB")
                color: Theme.textMuted
                font.pixelSize: Theme.captionSize
            }
        }
    }
}
