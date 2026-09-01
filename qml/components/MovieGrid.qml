pragma ComponentBehavior: Bound

import QtQuick
import Dostflix

GridView {
    id: root
    required property var movieModel
    signal releaseSelected(string title, string magnetUrl, string downloadUrl, string posterUrl)
    property int cardWidth: Theme.posterWidth
    property int cardGap: Theme.px(18)
    readonly property int columnCount: Math.max(1, Math.floor(width / (cardWidth + cardGap)))

    model: movieModel
    cellWidth: width / columnCount
    cellHeight: cardWidth / Theme.posterAspectRatio + Theme.px(82)
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    keyNavigationEnabled: true

    delegate: Item {
        required property var model
        width: root.cellWidth
        height: root.cellHeight

        MovieCard {
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.cardWidth
            title: parent.model.title
            year: parent.model.year
            quality: parent.model.quality
            seederCount: parent.model.seederCount
            posterUrl: parent.model.posterUrl
            sourceLabel: parent.model.sourceLabel
            onSelected: root.releaseSelected(parent.model.title, parent.model.magnetUrl,
                                             parent.model.downloadUrl, parent.model.posterUrl)
        }
    }
}
