import QtQuick
import QtQuick.Templates as T

T.ToolBar {
    id: control

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: implicitContentHeight + topPadding + bottomPadding
    background: Rectangle {
        color: "transparent"
    }
}