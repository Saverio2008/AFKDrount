import QtQuick
import QtQuick.Templates as T
import AppAppearance

T.SplitView {
    id: control

    implicitWidth: implicitContentWidth
    implicitHeight: implicitContentHeight

    handle: Rectangle {
        id: grip

        readonly property bool pressed: T.SplitHandle.pressed

        implicitWidth: Theme.gutter
        implicitHeight: Theme.gutter
        color: "transparent"

        Rectangle {
            anchors.centerIn: parent
            width: grip.width < grip.height ? 2 : 24
            height: grip.width < grip.height ? 24 : 2
            radius: 1
            color: grip.pressed ? Theme.primary : Theme.outline
        }
    }
}