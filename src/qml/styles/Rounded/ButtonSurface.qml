import QtQuick
import QtQuick.Templates
import AppAppearance

Rectangle {
    required property AbstractButton targetButton
    property bool primary: false
    property bool flat: false
    readonly property color fill: !targetButton.enabled ? Theme.disabledSurface : primary ? Theme.primary : targetButton.checked ? targetButton.palette.highlight : flat && !targetButton.hovered ? "transparent" : targetButton.palette.button

    implicitWidth: 88
    implicitHeight: Theme.buttonHeight
    radius: height / 2
    color: targetButton.down ? Qt.darker(fill, 1.12) : targetButton.hovered ? Qt.tint(fill, Theme.hoverTint) : fill
    border.width: targetButton.visualFocus ? 2 : 1
    border.color: targetButton.visualFocus ? Theme.primary : "transparent"
}