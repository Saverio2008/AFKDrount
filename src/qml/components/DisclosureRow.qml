import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T

T.ItemDelegate {
    id: control
    checkable: true
    implicitHeight: Math.max(32, implicitContentHeight + topPadding + bottomPadding)
    padding: 4
    focusPolicy: Qt.StrongFocus
    hoverEnabled: true
    Accessible.name: text
    contentItem: RowLayout {
        spacing: 6
        Label {
            text: control.checked ? "▾" : "▸"
            Accessible.ignored: true
        }
        Label {
            Layout.fillWidth: true
            text: control.text
            font.weight: Font.Medium
            elide: Text.ElideRight
        }
    }
    background: Rectangle {
        color: control.down || control.hovered ? control.palette.highlight : "transparent"
        border.width: control.visualFocus ? 1 : 0
        border.color: control.palette.highlight
        radius: 4
    }
}