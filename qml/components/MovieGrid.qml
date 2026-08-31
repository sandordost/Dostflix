pragma ComponentBehavior: Bound
import QtQuick
import Dostflix

GridView {
    id: root
    required property var movieModel
    property int cardWidth: 170
    model: movieModel
    cellWidth: Math.max(cardWidth + 12,
                        width / Math.max(1, Math.floor(width / (cardWidth + 12))))
    cellHeight: cardWidth / Theme.posterAspectRatio + 74
    clip: true

    delegate: MovieCard {
        required property var model
        width: root.cardWidth
        title: model.title
        year: model.year
        quality: model.quality
        seederCount: model.seederCount
        posterUrl: model.posterUrl
    }
}
