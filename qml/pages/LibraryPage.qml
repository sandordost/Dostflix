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
        if (!canResume) {
            libraryManager.play(index, true)
            return
        }
        resumeDialog.movieIndex = index
        resumeDialog.movieTitle = title
        resumeDialog.watchedSeconds = watchedSeconds
        resumeDialog.open()
    }

    Dialog {
        id: resumeDialog
        objectName: "resumePlaybackDialog"
        anchors.centerIn: parent
        width: Math.min(Theme.px(540), root.width - Theme.px(48))
        height: Theme.px(220)
        modal: true
        title: qsTr("Continue watching?")
        background: Rectangle { radius: Theme.radiusLarge; color: Theme.panel }
        property int movieIndex: -1
        property string movieTitle: ""
        property int watchedSeconds: 0

        contentItem: Label {
            text: qsTr("Resume %1 at %2, or start from the beginning?")
                    .arg(resumeDialog.movieTitle)
                    .arg(root.formatTime(resumeDialog.watchedSeconds))
            color: Theme.textPrimary
            wrapMode: Text.WordWrap
        }
        footer: DialogButtonBox {
            background: Item {}
            AppButton {
                objectName: "restartMovieButton"
                text: qsTr("Start over")
                DialogButtonBox.buttonRole: DialogButtonBox.ResetRole
                onClicked: {
                    resumeDialog.close()
                    root.libraryManager.play(resumeDialog.movieIndex, true)
                }
            }
            AppButton {
                objectName: "resumeMovieButton"
                text: qsTr("Resume")
                primary: true
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: {
                    resumeDialog.close()
                    root.libraryManager.play(resumeDialog.movieIndex, false)
                }
            }
        }
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
                radius: Theme.radius
                color: movieHover.hovered ? Theme.raisedHover : Theme.surface
                Accessible.role: Accessible.Button
                Accessible.name: movieRow.title
                Accessible.onPressAction: root.requestPlayback(
                    movieRow.index, movieRow.title,
                    movieRow.durationSeconds, movieRow.watchedSeconds)

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
                        onClicked: root.requestPlayback(
                            movieRow.index, movieRow.title,
                            movieRow.durationSeconds, movieRow.watchedSeconds)
                    }
                }

                HoverHandler { id: movieHover }
            }

            ScrollBar.vertical: ScrollBar {}
        }
    }
}
