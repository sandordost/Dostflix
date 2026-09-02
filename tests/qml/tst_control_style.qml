import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "ControlStyle"
    when: windowShown

    ApplicationWindow {
        width: 520
        height: 260
        visible: true
        color: Theme.canvas

        Column {
            anchors.centerIn: parent
            width: 360
            spacing: 12

            AppTextField {
                id: field
                width: parent.width
                placeholderText: "Search movies"
            }
            AppComboBox {
                id: combo
                width: parent.width
                model: ["Torznab", "Prowlarr"]
            }
            AppComboBox {
                id: emptyCombo
                visible: false
                model: []
            }
            AppButton {
                id: button
                text: "Save settings"
                icon.name: "document-open-symbolic"
            }
            AppToolButton {
                id: iconButton
                icon.name: "media-playback-start-symbolic"
            }
            PathPickerDialog {
                id: picker
                title: "Choose a file"
                fileNameFilters: ["*.ovpn"]
            }
        }
    }

    SignalSpy {
        id: buttonClickSpy
        target: button
        signalName: "clicked"
    }

    function init() {
        buttonClickSpy.clear()
    }

    function test_font_and_borderless_radius() {
        compare(field.font.family, "Montserrat")
        compare(field.background.radius, Theme.radius)
        compare(field.background.border.width, 0)
        compare(combo.background.radius, Theme.radius)
        compare(combo.background.border.width, 0)
    }

    function test_hover_and_popup_stay_dark() {
        mouseMove(button, button.width / 2, button.height / 2)
        tryVerify(function() { return button.hovered })
        tryCompare(button.background, "color", Theme.buttonHover, 300)
        verify(button.background.color !== "#ffffff")

        mouseClick(combo)
        tryCompare(combo.popup, "visible", true)
        compare(combo.popup.background.color, Theme.surface)
        combo.popup.close()
    }

    function test_font_awesome_mapping() {
        verify(Theme.iconGlyph("media-playback-start-symbolic").length > 0)
        verify(Theme.iconGlyph("preferences-system-symbolic").length > 0)
    }

    function test_icon_buttons_are_centered() {
        const row = iconButton.contentItem.children[0]
        verify(row !== null)
        compare(Math.round(row.x), Math.round((iconButton.contentItem.width - row.width) / 2))
    }

    function test_controller_activation_clicks_buttons() {
        button.controllerActivate()
        compare(buttonClickSpy.count, 1)
    }

    function test_empty_combo_does_not_open_for_controller() {
        emptyCombo.controllerActivate()
        compare(emptyCombo.popup.opened, false)
    }

    function test_file_picker_uses_app_surface() {
        compare(picker.background.color, Theme.panel)
        compare(picker.fileNameFilters[0], "*.ovpn")
    }
}
