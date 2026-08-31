import QtQuick
import QtTest
import Dostflix

TestCase {
    name: "Theme"

    function test_tokens_are_stable() {
        compare(Theme.posterAspectRatio, 2 / 3)
        compare(Theme.iconSize, 22)
        compare(Theme.panelOpacity, 0.87)
        verify(Theme.textPrimary !== Theme.panel)
    }
}
