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
    readonly property int columnCount: Math.max(1, Math.floor(width / (cardWidth + cardGap)))

    function releaseKey(title, magnetUrl, downloadUrl) {
        return title + "\n" + magnetUrl + "\n" + downloadUrl
    }

    function focusFirstResult() {
        if (count < 1)
            return false
        currentIndex = 0
        positionViewAtIndex(0, GridView.Beginning)
        Qt.callLater(function() {
            if (root.currentItem && root.currentItem.children.length > 0)
                root.currentItem.children[0].forceActiveFocus(Qt.TabFocusReason)
        })
        return true
    }

    model: movieModel
    cellWidth: width / columnCount
    cellHeight: cardWidth / Theme.posterAspectRatio + Theme.px(82)
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    keyNavigationEnabled: true

    delegate: Item {
        id: gridDelegate
        required property var model
        width: root.cellWidth
        height: root.cellHeight

        MovieCard {
            id: card
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.cardWidth
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
