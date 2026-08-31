import QtQuick
import QtQuick.Controls
import Dostflix

Item {
    Label {
        anchors.centerIn: parent
        text: qsTr("No active downloads")
        color: Theme.textSecondary
        font.pixelSize: Theme.bodySize
    }
}
