import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Dostflix

Item {
    id: root
    required property var movieModel
    required property var prowlarrManager

    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        Label {
            text: qsTr("Results")
            color: Theme.textPrimary
            font.pixelSize: Theme.titleSize
            font.weight: Font.Bold
        }
        Label {
            Layout.fillWidth: true
            visible: root.prowlarrManager.searchBusy || root.prowlarrManager.searchError.length > 0
            text: root.prowlarrManager.searchBusy
                  ? qsTr("Searching all configured indexers…")
                  : root.prowlarrManager.searchError
            color: root.prowlarrManager.searchError.length > 0 ? "#ff9b9b" : Theme.textSecondary
            wrapMode: Text.WordWrap
        }
        MovieGrid {
            Layout.fillWidth: true
            Layout.fillHeight: true
            movieModel: root.movieModel
        }
    }
}
