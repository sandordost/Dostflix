import QtQuick
import QtTest
import Dostflix

TestCase {
    name: "MovieCardGeometry"
    when: windowShown

    MovieCard {
        id: card
        title: "A very long movie title that must elide"
        year: 2026
        quality: "4K HDR"
        seederCount: 10
        posterUrl: ""
        sourceLabel: "Test indexer"
    }

    function test_fixed_geometry() {
        compare(card.width, 170)
        compare(Math.round((card.height - 62) / card.width * 100), 150)
    }
}
