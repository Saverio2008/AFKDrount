import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

RowLayout {
    spacing: 6

    ToolButton {
        text: qsTrId("appearance.system")
        checkable: true
        autoExclusive: true
        checked: Theme.mode === Theme.System
        onClicked: Theme.mode = Theme.System
    }
    ToolButton {
        text: qsTrId("appearance.light")
        checkable: true
        autoExclusive: true
        checked: Theme.mode === Theme.Light
        onClicked: Theme.mode = Theme.Light
    }
    ToolButton {
        text: qsTrId("appearance.dark")
        checkable: true
        autoExclusive: true
        checked: Theme.mode === Theme.Dark
        onClicked: Theme.mode = Theme.Dark
    }
}