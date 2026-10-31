import QtQuick
import QtQuick.Templates as T
import AppAppearance

T.ToolButton {
    id: control

    implicitWidth: Math.max(60, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(Theme.buttonHeight, implicitContentHeight + topPadding + bottomPadding)
    padding: 8
    horizontalPadding: 14
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? control.palette.buttonText : Theme.disabledText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: ButtonSurface {
        targetButton: control
        flat: false
    }
}