import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 1280
    height: 760
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: qsTr("Dostflix")
    color: "#05070c"

    Label {
        anchors.centerIn: parent
        text: qsTr("Dostflix")
        color: "white"
        font.pixelSize: 32
        font.weight: Font.Bold
    }
}
