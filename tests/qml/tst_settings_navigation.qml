import QtQuick
import QtQuick.Controls
import QtTest
import Dostflix

TestCase {
    name: "SettingsControllerNavigation"
    when: windowShown

    ListModel {
        id: profiles
        ListElement { name: "Profile A"; uuid: "uuid-a" }
        ListElement { name: "Profile B"; uuid: "uuid-b" }
    }
    ListModel { id: providers }

    QtObject {
        id: fakeVpn
        property var profileModel: profiles
        property string selectedProfileUuid: "uuid-a"
        property bool connected: false
        property bool ownsConnection: false
        property bool busy: false
        property string stateLabel: "Disconnected"
        property bool networkReady: false
        property string errorMessage: ""
        property string lastSelected: ""
        function selectProfile(uuid) { lastSelected = uuid; selectedProfileUuid = uuid }
        function importProfile(path) {}
        function refreshProfiles() {}
        function connectSelected() {}
        function disconnectOwned() {}
    }
    QtObject {
        id: fakeProviders
        property bool hasTmdbToken: true
        property string errorMessage: ""
        property var model: providers
        function saveTmdbToken(token) { return true }
        function clearTmdbToken() {}
        function addProvider(name, kind, endpoint, apiKey) { return true }
        function removeProvider(index) {}
    }
    QtObject {
        id: fakeProwlarr
        property bool running: true
        property bool ready: true
        property string stateLabel: "Prowlarr ready"
        property string errorMessage: ""
        function openWebInterface() {}
    }
    QtObject {
        id: fakeSubtitles
        property string preferredLanguages: "nl,en"
        property bool configured: true
        property string username: "tester"
        property string errorMessage: ""
        function setPreferredLanguages(value) {}
        function saveCredentials(apiKey, username, password) { return true }
        function clearCredentials() {}
    }
    QtObject {
        id: fakeLibrary
        property string directory: "/tmp/Movies"
        property string errorMessage: ""
        function setDirectory(path) {}
        function refresh() {}
    }
    QtObject {
        id: fakeController
        property bool connected: true
        property string controllerName: "Test controller"
        property int controllerCount: 1
        property string errorMessage: ""
    }

    ApplicationWindow {
        id: settingsWindow
        width: 1100
        height: 700
        visible: true

        SettingsPage {
            id: page
            anchors.fill: parent
            vpnManager: fakeVpn
            providerManager: fakeProviders
            prowlarrManager: fakeProwlarr
            subtitleManager: fakeSubtitles
            libraryManager: fakeLibrary
            controllerManager: fakeController
        }
    }

    function init() {
        settingsWindow.requestActivate()
        tryCompare(settingsWindow, "active", true)
    }

    function control(name) {
        const item = findChild(page, name)
        verify(item !== null, "Missing control " + name)
        return item
    }

    function focusControl(name) {
        const item = control(name)
        item.forceActiveFocus(Qt.TabFocusReason)
        tryCompare(item, "activeFocus", true)
        return item
    }

    function test_library_row_uses_left_and_right() {
        focusControl("libraryPathField")
        verify(page.handleControllerNavigation(1, 0))
        tryCompare(control("chooseLibraryButton"), "activeFocus", true)
        verify(page.handleControllerNavigation(1, 0))
        tryCompare(control("refreshLibraryButton"), "activeFocus", true)
    }

    function test_vertical_navigation_resets_to_first_item() {
        focusControl("chooseLibraryButton")
        verify(page.handleControllerNavigation(0, 1))
        tryCompare(control("configureIndexersButton"), "activeFocus", true)

        focusControl("removeTmdbButton")
        verify(page.handleControllerNavigation(0, 1))
        tryCompare(control("providerNameField"), "activeFocus", true)

        focusControl("providerApiKeyField")
        verify(page.handleControllerNavigation(0, 1))
        tryCompare(control("subtitleLanguagesField"), "activeFocus", true)

        focusControl("saveLanguagesButton")
        verify(page.handleControllerNavigation(0, 1))
        tryCompare(control("openSubtitlesApiKeyField"), "activeFocus", true)
    }

    function test_provider_row_reaches_every_field() {
        focusControl("providerNameField")
        const expected = ["providerKindBox", "providerEndpointField",
                          "providerApiKeyField", "addProviderButton"]
        for (let index = 0; index < expected.length; ++index) {
            verify(page.handleControllerNavigation(1, 0))
            tryCompare(control(expected[index]), "activeFocus", true)
        }
    }

    function test_open_combo_traps_navigation_until_selection() {
        const combo = focusControl("profileBox")
        combo.controllerActivate()
        tryCompare(combo.popup, "opened", true)
        compare(combo.controllerHighlightedIndex, 0)

        verify(page.handleControllerNavigation(0, 1))
        compare(combo.controllerHighlightedIndex, 1)
        verify(page.handleControllerNavigation(1, 0))
        compare(combo.controllerHighlightedIndex, 1)

        verify(page.activateControllerPopup())
        tryCompare(combo.popup, "opened", false)
        compare(fakeVpn.lastSelected, "uuid-b")
    }
}
