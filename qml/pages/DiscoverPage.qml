import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property var movieModel

    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        Label {
            text: qsTr("Results")
            color: Theme.textPrimary
            font.pixelSize: Theme.titleSize
            font.weight: Font.Bold
        }
        MovieGrid {
            Layout.fillWidth: true
            Layout.fillHeight: true
            movieModel: root.movieModel
        }
    }
}
