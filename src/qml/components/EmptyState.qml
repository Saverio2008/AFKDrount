import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

ColumnLayout {
    id: root

    required property string title
    required property string description

    spacing: 8

    Label {
        Layout.fillWidth: true
        text: root.title
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }

    Label {
        Layout.fillWidth: true
        text: root.description
        color: Theme.muted
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }
}