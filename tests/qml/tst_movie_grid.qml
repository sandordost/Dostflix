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
        compare(grid.columnCount, 4)
        compare(Math.floor(grid.width / grid.cellWidth), 4)
    }

    function test_grid_reflows_at_small_and_large_widths() {
        grid.width = 520
        compare(grid.columnCount, 2)
        verify(grid.cellWidth >= grid.cardWidth)

        grid.width = 1280
        compare(grid.columnCount, 6)
        verify(grid.cellWidth >= grid.cardWidth)

        grid.width = 880
    }
}
