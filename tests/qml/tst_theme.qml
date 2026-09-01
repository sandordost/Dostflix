import QtQuick
import QtTest
import Dostflix

TestCase {
    name: "Theme"

    function test_tokens_are_stable() {
        compare(Theme.posterAspectRatio, 2 / 3)
        compare(Theme.posterWidth, 170)
        compare(Theme.iconSize, 20)
        compare(Theme.panelOpacity, 0.91)
        verify(Theme.motionFast <= 150)
        verify(Theme.motionNormal <= 200)
        verify(Theme.controlsTimeout >= 2500)
        verify(Theme.textPrimary !== Theme.panel)
    }
}
