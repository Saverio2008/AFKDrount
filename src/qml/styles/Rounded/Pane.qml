import QtQuick
import QtQuick.Templates as T
import AppAppearance

T.Pane {
    id: control

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: implicitContentHeight + topPadding + bottomPadding
    padding: Theme.panelPadding

    background: Rectangle {
        color: control.palette.window
        radius: Theme.panelRadius
        border.color: Theme.outline
    }
}