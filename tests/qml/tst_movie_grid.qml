import QtQuick
import QtTest
import Dostflix

TestCase {
    name: "MovieGridWrapping"

    MovieGrid {
        id: grid
        width: 880
        height: 500
        movieModel: []
    }

    function test_cell_fits_width() {
        verify(grid.cellWidth >= grid.cardWidth)
        compare(Math.floor(grid.width / grid.cellWidth), 4)
    }
}
