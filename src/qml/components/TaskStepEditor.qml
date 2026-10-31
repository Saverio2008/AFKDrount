import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    required property int index
    required property string mode
    required property string content
    required property real waitSeconds
    property int stepCount: 1
    property bool revealed: false
    signal edited(string key, var value)
    signal moveRequested(int delta)
    signal removeRequested

    spacing: 6
    RowLayout {
        Layout.fillWidth: true
        Label {
            text: qsTrId("task.step.number").arg(root.index + 1)
            font.weight: Font.Medium
        }
        ComboBox {
            id: modeField
            readonly property var scrollablePopup: popup.contentItem
            Component.onCompleted: scrollablePopup.boundsBehavior = Flickable.StopAtBounds
            textRole: "label"
            model: ListModel {
                ListElement {
                    label: qsTrId("task.mode.chat")
                }
                ListElement {
                    label: qsTrId("task.mode.command")
                }
            }
            currentIndex: root.mode === "command" ? 1 : 0
            Accessible.name: qsTrId("task.mode")
            onActivated: root.edited("mode", currentIndex === 1 ? "command" : "chat")
        }
        Item {
            Layout.fillWidth: true
        }
        ToolButton {
            text: "↑"
            Accessible.name: qsTrId("task.up")
            enabled: root.index > 0
            onClicked: root.moveRequested(-1)
        }
        ToolButton {
            text: "↓"
            Accessible.name: qsTrId("task.down")
            enabled: root.index + 1 < root.stepCount
            onClicked: root.moveRequested(1)
        }
        ToolButton {
            text: qsTrId("action.delete")
            enabled: root.stepCount > 1
            onClicked: root.removeRequested()
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Label {
            text: qsTrId("task.delay")
        }
        CompactTextField {
            Layout.preferredWidth: 110
            text: Number.isFinite(root.waitSeconds) ? String(root.waitSeconds) : ""
            placeholderText: qsTrId("input.example.number").arg(0)
            validator: DoubleValidator {
                bottom: 0
                top: 86400
                decimals: 3
                locale: "C"
            }
            Accessible.name: qsTrId("task.delay")
            onTextEdited: root.edited("waitSeconds", acceptableInput && text.length > 0 ? Number(text) : NaN)
        }
        Label {
            text: qsTrId("task.seconds")
        }
        Item {
            Layout.fillWidth: true
        }
        ToolButton {
            visible: root.mode === "command"
            text: checked ? qsTrId("task.hide") : qsTrId("task.show")
            checkable: true
            checked: root.revealed
            onClicked: root.revealed = checked
        }
    }
    CompactTextField {
        Layout.fillWidth: true
        text: root.content
        maximumLength: 256
        placeholderText: root.mode === "command" ? qsTrId("task.command.placeholder") : qsTrId("task.chat.placeholder")
        Accessible.name: qsTrId("task.content")
        echoMode: root.mode === "command" && !root.revealed ? TextInput.Password : TextInput.Normal
        onTextEdited: root.edited("content", text)
    }
}
