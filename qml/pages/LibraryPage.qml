import QtQuick
import QtQuick.Controls
import Dostflix

Item {
    Label {
        anchors.centerIn: parent
        text: qsTr("Your library is empty")
        color: Theme.textSecondary
        font.pixelSize: Theme.bodySize
    }
}
