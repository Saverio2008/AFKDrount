import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import AppAppearance

T.TextField {
    id: control

    implicitWidth: 220
    implicitHeight: Math.max(Theme.fieldHeight, contentHeight + topPadding + bottomPadding)
    activeFocusOnTab: true
    padding: 10
    leftPadding: 18
    rightPadding: 18
    color: enabled ? Theme.text : Theme.disabledText
    placeholderTextColor: Theme.muted
    selectionColor: Theme.selection
    selectedTextColor: Theme.text

    PlaceholderText {
        x: control.leftPadding
        y: control.topPadding
        width: control.width - control.leftPadding - control.rightPadding
        height: control.height - control.topPadding - control.bottomPadding
        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText
                 && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
    }

    background: Rectangle {
        color: control.enabled ? Theme.field : Theme.disabledSurface
        radius: Theme.fieldRadius
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? Theme.primary : Theme.inputOutline
    }
}
