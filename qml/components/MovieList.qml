pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

ListView {
    id: root
    required property var movieModel
    signal releaseSelected(string title, string magnetUrl, string downloadUrl, string posterUrl)

    function focusFirstResult() {
        if (count < 1)
            return false
        currentIndex = 0
        positionViewAtIndex(0, ListView.Beginning)
        Qt.callLater(function() {
            if (root.currentItem)
                root.currentItem.forceActiveFocus(Qt.TabFocusReason)
        })
        return true
    }

    model: movieModel
    spacing: Theme.px(7)
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    keyNavigationEnabled: true

    delegate: Rectangle {
        id: releaseRow
        required property var model
        width: root.width
        height: Theme.px(92)
        activeFocusOnTab: true
        radius: Theme.radius
        color: rowHover.hovered || activeFocus ? Theme.raisedHover : Theme.surface
        border.width: activeFocus ? Theme.px(2) : 0
        border.color: Theme.accent
        Accessible.role: Accessible.Button
        Accessible.name: model.title
        Accessible.onPressAction: root.releaseSelected(
            model.title, model.magnetUrl, model.downloadUrl, model.posterUrl)

        function controllerActivate() {
            root.releaseSelected(model.title, model.magnetUrl, model.downloadUrl, model.posterUrl)
        }

        Behavior on color { ColorAnimation { duration: Theme.motionFast } }

        RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.px(8)
            spacing: Theme.px(14)

            Rectangle {
                Layout.preferredWidth: Theme.px(50)
                Layout.fillHeight: true
                radius: Theme.radiusSmall
                color: Theme.raised
                clip: true
                Image {
                    anchors.fill: parent
                    source: releaseRow.model.posterUrl.toString().length > 0
                            ? releaseRow.model.posterUrl
                            : "qrc:/qt/qml/Dostflix/assets/images/poster-placeholder.svg"
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    cache: true
                    sourceSize: Qt.size(Theme.px(100), Theme.px(152))
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.px(4)
                Label {
                    Layout.fillWidth: true
                    text: releaseRow.model.title
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.bodySize
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                Label {
                    Layout.fillWidth: true
                    text: (releaseRow.model.year > 0 ? releaseRow.model.year : qsTr("Unknown year"))
                          + (releaseRow.model.seederCount > 0
                             ? qsTr(" · %1 seeds").arg(releaseRow.model.seederCount) : "")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.captionSize
                    elide: Text.ElideRight
                }
                Label {
                    Layout.fillWidth: true
                    text: releaseRow.model.sourceLabel
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.captionSize
                    elide: Text.ElideRight
                }
            }

            Label {
                visible: releaseRow.model.quality.length > 0
                text: releaseRow.model.quality
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.captionSize
                font.weight: Font.DemiBold
                padding: Theme.px(9)
                background: Rectangle {
                    radius: Theme.radius
                    color: Theme.raised
                }
            }

            AppToolButton {
                icon.name: "media-playback-start-symbolic"
                Accessible.name: qsTr("Start %1").arg(releaseRow.model.title)
                focusPolicy: Qt.NoFocus
                activeFocusOnTab: false
                onClicked: root.releaseSelected(
                    releaseRow.model.title, releaseRow.model.magnetUrl,
                    releaseRow.model.downloadUrl, releaseRow.model.posterUrl)
            }
        }

        HoverHandler { id: rowHover }
        TapHandler {
            onTapped: root.releaseSelected(
                releaseRow.model.title, releaseRow.model.magnetUrl,
                releaseRow.model.downloadUrl, releaseRow.model.posterUrl)
        }
        Keys.onPressed: event => {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                    || event.key === Qt.Key_Space) {
                releaseRow.controllerActivate()
                event.accepted = true
            }
        }
    }

    ScrollBar.vertical: ScrollBar {}
}
