pragma ComponentBehavior: Bound

import QtQuick
import Dostflix

GridView {
    id: root
    required property var movieModel
    signal releaseSelected(string title, string magnetUrl, string downloadUrl, string posterUrl)
    property string selectedReleaseKey: ""
    property bool transferLoading: false
    property string transferStatusText: ""
    property bool transferHasError: false
    property int cardWidth: Theme.posterWidth
    property int cardGap: Theme.px(18)
    readonly property var controllerCurrentItem: currentItem
    readonly property int columnCount: Math.max(1, Math.floor(width / (cardWidth + cardGap)))

    function releaseKey(title, magnetUrl, downloadUrl) {
        return title + "\n" + magnetUrl + "\n" + downloadUrl
    }

    function focusFirstResult() {
        if (count < 1)
            return false
        positionViewAtIndex(0, GridView.Beginning)
        return focusIndex(0)
    }

    function ensureIndexVisible(index) {
        if (index < 0 || index >= count)
            return false
        currentIndex = index
        positionViewAtIndex(index, GridView.Contain)
        return true
    }

    function ownsActiveFocus() {
        let item = root.Window.window ? root.Window.window.activeFocusItem : null
        while (item) {
            if (item === root)
                return true
            item = item.parent
        }
        return false
    }

    function focusIndex(index) {
        if (!ensureIndexVisible(index))
            return false
        forceActiveFocus(Qt.TabFocusReason)
        return true
    }

    function controllerActivate() {
        if (controllerCurrentItem)
            controllerCurrentItem.controllerActivate()
    }

    function handleControllerNavigation(horizontal, vertical) {
        if (!ownsActiveFocus() || count < 1)
            return false
        let nextIndex = currentIndex
        if (horizontal !== 0) {
            const column = currentIndex % columnCount
            if (horizontal < 0 && column > 0)
                nextIndex -= 1
            else if (horizontal > 0 && column < columnCount - 1
                     && currentIndex + 1 < count)
                nextIndex += 1
        } else if (vertical !== 0) {
            const candidate = currentIndex + (vertical > 0
                                               ? columnCount : -columnCount)
            if (candidate >= 0 && candidate < count)
                nextIndex = candidate
        }
        if (nextIndex !== currentIndex)
            focusIndex(nextIndex)
        return true
    }

    objectName: "movieGrid"
    model: movieModel
    cellWidth: width / columnCount
    cellHeight: cardWidth / Theme.posterAspectRatio + Theme.px(82)
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    keyNavigationEnabled: true
    activeFocusOnTab: true
    cacheBuffer: cellHeight * 2

    delegate: Item {
        id: gridDelegate
        required property int index
        required property var model
        width: root.cellWidth
        height: root.cellHeight
        activeFocusOnTab: false
        Accessible.role: Accessible.Button
        Accessible.name: model.title

        function controllerActivate() {
            card.controllerActivate()
        }

        MovieCard {
            id: card
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.cardWidth
            activeFocusOnTab: false
            controllerFocused: root.activeFocus
                               && root.currentIndex === gridDelegate.index
            title: parent.model.title
            year: parent.model.year
            quality: parent.model.quality
            seederCount: parent.model.seederCount
            posterUrl: parent.model.posterUrl
            sourceLabel: parent.model.sourceLabel
            transferActive: root.selectedReleaseKey === root.releaseKey(
                                parent.model.title, parent.model.magnetUrl,
                                parent.model.downloadUrl)
            transferLoading: transferActive && root.transferLoading
            transferStatusText: transferActive ? root.transferStatusText : ""
            transferHasError: transferActive && root.transferHasError
            onSelected: root.releaseSelected(parent.model.title, parent.model.magnetUrl,
                                             parent.model.downloadUrl, parent.model.posterUrl)
        }
    }
}
