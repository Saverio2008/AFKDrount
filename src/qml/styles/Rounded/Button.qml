import QtQuick
import QtQuick.Templates as T
import AppAppearance

T.Button {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight, implicitContentHeight + topPadding + bottomPadding)
    padding: 10
    horizontalPadding: 20
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    contentItem: Text {
        text: control.text
        font: control.font
        color: !control.enabled ? Theme.disabledText : control.highlighted ? Theme.primaryText : control.palette.buttonText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: ButtonSurface {
        targetButton: control
        primary: control.highlighted
        flat: control.flat
    }
}