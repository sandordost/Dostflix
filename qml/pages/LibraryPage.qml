pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property var libraryManager
    required property var metadataManager

    function formatTime(seconds) {
        const total = Math.max(0, Math.floor(seconds))
        const hours = Math.floor(total / 3600)
        const minutes = Math.floor((total % 3600) / 60)
        const remainder = total % 60
        return hours > 0
                ? hours + ":" + String(minutes).padStart(2, "0") + ":" + String(remainder).padStart(2, "0")
                : minutes + ":" + String(remainder).padStart(2, "0")
    }

    function requestPlayback(index, title, durationSeconds, watchedSeconds) {
        const canResume = watchedSeconds >= 30
                && (durationSeconds <= 0 || durationSeconds - watchedSeconds > 60)
        libraryManager.play(index, !canResume)
    }

    function focusFirstControl() {
        libraryRefreshButton.forceActiveFocus(Qt.TabFocusReason)
        return true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.px(12)

        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: qsTr("Library")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.headingSize
                font.weight: Font.DemiBold
            }
            Label {
                text: qsTr("%1 movies").arg(root.libraryManager.count)
                color: Theme.textSecondary
                font.family: Theme.fontFamily
            }
            AppToolButton {
                id: libraryRefreshButton
                objectName: "libraryRefreshButton"
                icon.name: "view-refresh-symbolic"
                Accessible.name: qsTr("Rescan movie library")
                onClicked: root.libraryManager.refresh()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.metadataManager.busy || root.metadataManager.errorMessage.length > 0
            BusyIndicator {
                implicitWidth: Theme.px(22)
                implicitHeight: Theme.px(22)
                running: root.metadataManager.busy
                visible: running
            }
            Label {
                Layout.fillWidth: true
                text: root.metadataManager.busy
                      ? root.metadataManager.stateLabel : root.metadataManager.errorMessage
                color: root.metadataManager.errorMessage.length > 0
                       ? Theme.danger : Theme.textSecondary
                elide: Text.ElideRight
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillHeight: true
            visible: root.libraryManager.count === 0
            text: qsTr("Your library is empty\nAdd video files to %1").arg(root.libraryManager.directory)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.bodySize
        }

        ListView {
            id: libraryList
            objectName: "libraryList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.libraryManager.count > 0
            clip: true
            spacing: Theme.px(8)
            boundsBehavior: Flickable.StopAtBounds
            model: root.libraryManager.model

            delegate: Rectangle {
                id: movieRow
                required property int index
                required property string title
                required property url posterUrl
                required property int year
                required property int durationSeconds
                required property int watchedSeconds
                width: libraryList.width
                height: Theme.px(132)
                activeFocusOnTab: true
                radius: Theme.radius
                color: movieHover.hovered || activeFocus ? Theme.raisedHover : Theme.surface
                border.width: activeFocus ? Theme.px(2) : 0
                border.color: Theme.accent
                Accessible.role: Accessible.Button
                Accessible.name: movieRow.title
                Accessible.onPressAction: root.requestPlayback(
                    movieRow.index, movieRow.title,
                    movieRow.durationSeconds, movieRow.watchedSeconds)

                function controllerActivate() {
                    root.requestPlayback(movieRow.index, movieRow.title,
                                         movieRow.durationSeconds, movieRow.watchedSeconds)
                }

                Behavior on color { ColorAnimation { duration: Theme.motionFast } }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.px(9)
                    spacing: Theme.px(16)

                    Rectangle {
                        Layout.preferredWidth: Theme.px(76)
                        Layout.fillHeight: true
                        radius: Theme.radiusSmall
                        color: Theme.raised
                        clip: true
                        Image {
                            anchors.fill: parent
                            source: movieRow.posterUrl.toString().length > 0
                                    ? movieRow.posterUrl
                                    : "qrc:/qt/qml/Dostflix/assets/images/poster-placeholder.svg"
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: true
                            sourceSize: Qt.size(Theme.px(152), Theme.px(228))
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.px(6)
                        Label {
                            Layout.fillWidth: true
                            text: movieRow.title
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.headingSize
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }
                        Label {
                            Layout.fillWidth: true
                            text: (movieRow.year > 0 ? movieRow.year : qsTr("Unknown year"))
                                  + (movieRow.durationSeconds > 0
                                     ? qsTr(" · %1 min").arg(Math.round(movieRow.durationSeconds / 60)) : "")
                            color: Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.bodySize
                            elide: Text.ElideRight
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: movieRow.watchedSeconds >= 30
                            text: qsTr("Watched to %1").arg(root.formatTime(movieRow.watchedSeconds))
                            color: Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.captionSize
                        }
                    }

                    AppButton {
                        objectName: "libraryPlayButton"
                        text: movieRow.watchedSeconds >= 30
                              ? qsTr("Continue · %1").arg(root.formatTime(movieRow.watchedSeconds))
                              : qsTr("Play")
                        icon.name: "media-playback-start-symbolic"
                        primary: true
                        activeFocusOnTab: false
                        focusPolicy: Qt.NoFocus
                        onClicked: root.requestPlayback(
                            movieRow.index, movieRow.title,
                            movieRow.durationSeconds, movieRow.watchedSeconds)
                    }
                }

                HoverHandler { id: movieHover }
                Keys.onPressed: event => {
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                            || event.key === Qt.Key_Space) {
                        movieRow.controllerActivate()
                        event.accepted = true
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }
    }
}
