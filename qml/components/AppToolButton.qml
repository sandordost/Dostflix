import QtQuick
import Dostflix

AppButton {
    id: root
    property bool round: false
    implicitWidth: 42
    width: implicitWidth
    leftPadding: 10
    rightPadding: 10
    cornerRadius: round ? height / 2 : Theme.radius
}
