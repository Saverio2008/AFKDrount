import QtQuick
import QtQuick.Templates as T
import AppAppearance

T.Label {
    id: control

    color: control.enabled ? control.palette.windowText : Theme.disabledText
    linkColor: Theme.primary
}