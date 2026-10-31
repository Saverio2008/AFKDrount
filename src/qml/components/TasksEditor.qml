import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

ColumnLayout {
    id: root
    required property var tasks
    property string gameVersion: ""
    property var editingTask: null
    property int editingIndex: -1
    signal edited(var value)

    function replaceTask(index, value) {
        const copy = root.tasks.slice();
        copy[index] = value;
        root.edited(copy);
    }
    function editTask(index) {
        root.editingIndex = index;
        root.editingTask = index < 0 ? InstanceStore.newTask() : JSON.parse(JSON.stringify(root.tasks[index]));
    }
    function moveTask(index, delta) {
        const copy = root.tasks.slice();
        const entry = copy.splice(index, 1)[0];
        copy.splice(index + delta, 0, entry);
        root.edited(copy);
    }
    function copyTask(index) {
        const copy = JSON.parse(JSON.stringify(root.tasks[index]));
        copy.id = InstanceStore.newTask().id;
        const base = copy.name + qsTrId("instance.copy.suffix");
        copy.name = base.slice(0, 80);
        let number = 2;
        while (root.tasks.some(task => task.name === copy.name)) {
            const suffix = " " + number++;
            copy.name = base.slice(0, 80 - suffix.length) + suffix;
        }
        root.edited(root.tasks.concat([copy]));
    }

    spacing: 8
    RowLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: qsTrId("task.title")
            font.weight: Font.Medium
        }
        ToolButton {
            text: qsTrId("task.add")
            enabled: GameCatalog.supports(root.gameVersion) && root.tasks.length < 64
            onClicked: root.editTask(-1)
        }
    }
    Label {
        Layout.fillWidth: true
        visible: root.tasks.length === 0
        text: qsTrId("task.empty")
        color: Theme.muted
        wrapMode: Text.WordWrap
    }
    Repeater {
        model: root.tasks
        delegate: RowLayout {
            id: row
            required property var modelData
            required property int index
            readonly property var editor: parent
            Layout.fillWidth: true
            spacing: 2
            CheckBox {
                checked: row.modelData.enabled
                Accessible.name: qsTrId("config.rule.toggle").arg(row.modelData.name)
                onClicked: {
                    const copy = JSON.parse(JSON.stringify(row.modelData));
                    copy.enabled = checked;
                    row.editor.replaceTask(row.index, copy);
                }
            }
            Label {
                Layout.fillWidth: true
                text: row.modelData.name
                elide: Text.ElideRight
            }
            ToolButton {
                text: "↑"
                Accessible.name: qsTrId("task.up")
                enabled: row.index > 0
                onClicked: row.editor.moveTask(row.index, -1)
            }
            ToolButton {
                text: "↓"
                Accessible.name: qsTrId("task.down")
                enabled: row.index + 1 < row.editor.tasks.length
                onClicked: row.editor.moveTask(row.index, 1)
            }
            ToolButton {
                text: qsTrId("action.edit")
                onClicked: row.editor.editTask(row.index)
            }
            ToolButton {
                text: qsTrId("config.rule.copy")
                enabled: row.editor.tasks.length < 64
                onClicked: row.editor.copyTask(row.index)
            }
            ToolButton {
                text: qsTrId("action.delete")
                onClicked: row.editor.edited(row.editor.tasks.filter((task, index) => index !== row.index))
            }
        }
    }
    Loader {
        id: dialogLoader
        active: root.editingTask !== null
        readonly property var popup: status === Loader.Ready ? item : null
        sourceComponent: TaskDialog {}
        onLoaded: {
            if (status === Loader.Ready && popup) {
                popup.task = root.editingTask;
                popup.gameVersion = root.gameVersion;
                popup.open();
            }
        }
    }
    Connections {
        target: dialogLoader.popup
        function onTaskAccepted(value) {
            if (root.editingIndex < 0)
                root.edited(root.tasks.concat([value]));
            else
                root.replaceTask(root.editingIndex, value);
        }
        function onClosed() {
            root.editingTask = null;
        }
    }
}
