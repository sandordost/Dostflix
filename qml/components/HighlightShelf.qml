pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Dostflix

Item {
    id: root
    required property string title
    required property var movieModel
    signal movieSelected(string title)
    property alias currentIndex: movieList.currentIndex
    readonly property int count: movieList.count
    implicitHeight: heading.implicitHeight + Theme.px(12) + movieList.height

    function focusIndex(index) {
        if (movieList.count < 1)
            return false
        movieList.currentIndex = Math.max(0, Math.min(movieList.count - 1, index))
        movieList.positionViewAtIndex(movieList.currentIndex, ListView.Contain)
        movieList.forceActiveFocus(Qt.TabFocusReason)
        return true
    }

    function handleHorizontal(direction) {
        if (direction === 0 || movieList.count < 1)
            return false
        return focusIndex(movieList.currentIndex + (direction > 0 ? 1 : -1))
    }

    function activateCurrent() {
        if (movieList.currentIndex < 0 || movieList.currentIndex >= movieList.count)
            return false
        let selectedTitle = ""
        if (typeof root.movieModel.titleAt === "function")
            selectedTitle = root.movieModel.titleAt(movieList.currentIndex)
        else if (typeof root.movieModel.get === "function")
            selectedTitle = root.movieModel.get(movieList.currentIndex).title
        if (selectedTitle.length === 0)
            return false
        root.movieSelected(selectedTitle)
        return true
    }

    Label {
        id: heading
        anchors.left: parent.left
        anchors.top: parent.top
        text: root.title
        color: Theme.textPrimary
        font.pixelSize: Theme.headingSize
        font.weight: Font.DemiBold
    }

    ListView {
        id: movieList
        objectName: "highlightShelfList"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: heading.bottom
        anchors.topMargin: Theme.px(12)
        height: Theme.posterWidth / Theme.posterAspectRatio + Theme.px(78)
        orientation: ListView.Horizontal
        spacing: Theme.px(18)
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: root.movieModel
        activeFocusOnTab: true
        keyNavigationEnabled: false

        function controllerActivate() {
            root.activateCurrent()
        }

        delegate: MovieCard {
            required property int index
            required property var model
            controllerFocused: movieList.activeFocus && movieList.currentIndex === index
            title: model.title
            year: model.year
            quality: ""
            seederCount: 0
            rating: model.rating
            posterUrl: model.posterUrl
            sourceLabel: qsTr("Movie data from TMDB")
            actionSymbol: "\uf002"
            onSelected: root.movieSelected(model.title)
        }
    }
}
