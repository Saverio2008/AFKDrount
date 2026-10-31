import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

ColumnLayout {
    id: root

    required property var rules
    property string gameVersion: ""
    property var editingRule: null
    property int editingIndex: -1
    signal edited(var value)

    function replaceRule(index, value) {
        const updated = root.rules.slice();
        updated[index] = value;
        root.edited(updated);
    }

    function editRule(index: int) {
        root.editingIndex = index;
        root.editingRule = index < 0 ? InstanceStore.newCountingRule() : JSON.parse(JSON.stringify(root.rules[index]));
    }

    spacing: 8

    RowLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: qsTrId("config.rules.title")
            font.weight: Font.Medium
        }
        ToolButton {
            text: qsTrId("config.rule.add")
            enabled: GameCatalog.supports(root.gameVersion)
            onClicked: root.editRule(-1)
        }
    }
    Label {
        Layout.fillWidth: true
        visible: root.rules.length === 0
        text: qsTrId("config.rules.empty")
        color: Theme.muted
        wrapMode: Text.WordWrap
    }
    Repeater {
        model: root.rules
        delegate: RowLayout {
            id: row
            required property var modelData
            required property int index
            readonly property CountingRulesEditor editor: parent
            Layout.fillWidth: true
            spacing: 4
            CheckBox {
                checked: row.modelData.enabled
                Accessible.name: qsTrId("config.rule.toggle").arg(row.modelData.name)
                onClicked: {
                    const value = JSON.parse(JSON.stringify(row.modelData));
                    value.enabled = checked;
                    row.editor.replaceRule(row.index, value);
                }
            }
            Label {
                Layout.fillWidth: true
                text: row.modelData.name
                elide: Text.ElideRight
                ToolTip.visible: hovered.hovered
                ToolTip.text: row.modelData.items.join(", ")
                HoverHandler {
                    id: hovered
                }
            }
            ToolButton {
                text: qsTrId("action.edit")
                enabled: GameCatalog.supports(row.editor.gameVersion)
                onClicked: row.editor.editRule(row.index)
            }
            ToolButton {
                text: qsTrId("config.rule.copy")
                onClicked: {
                    const copy = JSON.parse(JSON.stringify(row.modelData));
                    copy.id = InstanceStore.newCountingRule().id;
                    const base = copy.name + qsTrId("instance.copy.suffix");
                    copy.name = base.slice(0, 80);
                    let number = 2;
                    while (row.editor.rules.some(rule => rule.name === copy.name)) {
                        const suffix = " " + number++;
                        copy.name = base.slice(0, 80 - suffix.length) + suffix;
                    }
                    row.editor.edited(row.editor.rules.concat([copy]));
                }
            }
            ToolButton {
                text: qsTrId("action.delete")
                onClicked: row.editor.edited(row.editor.rules.filter((rule, index) => index !== row.index))
            }
        }
    }
    Loader {
        id: dialogLoader
        active: root.editingRule !== null
        readonly property var popup: status === Loader.Ready ? item : null
        sourceComponent: CountingRuleDialog {}
        onLoaded: {
            if (status === Loader.Ready && popup) {
                popup.rule = root.editingRule;
                popup.gameVersion = root.gameVersion;
                popup.open();
            }
        }
    }
    Connections {
        target: dialogLoader.popup
        function onRuleAccepted(value) {
            if (root.editingIndex < 0)
                root.edited(root.rules.concat([value]));
            else
                root.replaceRule(root.editingIndex, value);
        }
        function onClosed() {
            root.editingRule = null;
        }
    }
}
