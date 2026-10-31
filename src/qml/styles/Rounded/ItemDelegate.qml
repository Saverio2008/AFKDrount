import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import AppAppearance

T.ItemDelegate {
    id: control

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: Math.max(68, implicitContentHeight + topPadding + bottomPadding)
    padding: 14
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    spacing: 12
    readonly property string heading: text.split("\n")[0]
    readonly property string detail: text.indexOf("\n") >= 0 ? text.slice(text.indexOf("\n") + 1) : ""

    contentItem: RowLayout {
        spacing: control.spacing

        Rectangle {
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            radius: 10
            color: control.palette.highlight
            Accessible.ignored: true

            Text {
                anchors.centerIn: parent
                visible: thumbnail.status !== Image.Ready
                text: control.heading.slice(0, 1)
                font: control.font
                color: Theme.text
            }
            Image {
                id: thumbnail
                anchors.centerIn: parent
                width: 32
                height: 32
                //noinspection UnnecessaryLabelJS
                source: control.icon.source
                fillMode: Image.PreserveAspectFit
                smooth: false
                visible: status === Image.Ready
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3

            Text {
                Layout.fillWidth: true
                text: control.heading
                font.family: control.font.family
                font.pointSize: control.font.pointSize
                font.weight: Font.Medium
                color: control.enabled ? Theme.text : Theme.disabledText
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                visible: control.detail.length > 0
                text: control.detail
                font.family: control.font.family
                font.pointSize: Math.max(10, control.font.pointSize - 1)
                color: Theme.muted
                elide: Text.ElideRight
            }
        }
    }
    background: Rectangle {
        radius: Theme.rowRadius
        color: control.highlighted ? control.palette.highlight : control.hovered ? Qt.tint(control.palette.window, Theme.hoverTint) : control.palette.window
        border.width: control.visualFocus ? 2 : 1
        border.color: control.visualFocus ? Theme.primary : Theme.outline
    }
}