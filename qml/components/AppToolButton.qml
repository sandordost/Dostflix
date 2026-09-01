import QtQuick
import Dostflix

AppButton {
    id: root
    property bool round: false
    implicitWidth: Theme.px(42)
    width: implicitWidth
    leftPadding: Theme.px(10)
    rightPadding: Theme.px(10)
    cornerRadius: round ? height / 2 : Theme.radius
}
