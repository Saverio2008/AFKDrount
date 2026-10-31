import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

ColumnLayout {
    id: root
    property var items: []
    signal edited(var value)
    spacing: 6

    ToolButton {
        id: expanded
        checkable: true
        text: (checked ? "▾ " : "▸ ") + qsTrId("catalog.custom.title")
    }
    ColumnLayout {
        Layout.fillWidth: true
        visible: expanded.checked
        Label {
            Layout.fillWidth: true
            text: qsTrId("catalog.custom.hint")
            color: Theme.muted
            wrapMode: Text.WordWrap
        }
        ComboBox {
            id: source
            readonly property var scrollablePopup: popup.contentItem
            Component.onCompleted: scrollablePopup.boundsBehavior = Flickable.StopAtBounds
            Layout.fillWidth: true
            model: ListModel {
                ListElement {
                    sourceName: "nbt"
                }
                ListElement {
                    sourceName: "component"
                }
            }
            displayText: currentIndex === 0 ? qsTrId("catalog.custom.nbt") : qsTrId("catalog.custom.component")
            delegate: ItemDelegate {
                required property int index
                text: index === 0 ? qsTrId("catalog.custom.nbt") : qsTrId("catalog.custom.component")
            }
        }
        CompactTextField {
            id: path
            Layout.fillWidth: true
            placeholderText: qsTrId("catalog.custom.path")
            Accessible.name: placeholderText
            maximumLength: 1024
        }
        RowLayout {
            Layout.fillWidth: true
            CompactTextField {
                id: identifier
                Layout.fillWidth: true
                placeholderText: qsTrId("catalog.custom.id")
                Accessible.name: placeholderText
                maximumLength: 256
            }
            ToolButton {
                text: qsTrId("config.rule.add")
                enabled: identifier.text.length > 0 && /^(?:\/(?:[^~/]|~[01])*)+$/.test(path.text) && root.items.length < 128
                onClicked: {
                    const matcher = {
                        id: identifier.text,
                        source: source.currentIndex === 0 ? "nbt" : "component",
                        path: path.text
                    };
                    if (!root.items.some(item => item.id === matcher.id && item.source === matcher.source && item.path === matcher.path))
                        root.edited(root.items.concat([matcher]));
                    identifier.clear();
                }
            }
        }
    }
    Repeater {
        model: root.items
        delegate: RowLayout {
            id: entry
            required property var modelData
            required property int index
            readonly property var editor: parent
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: entry.modelData.id + " · " + entry.modelData.source + " " + entry.modelData.path
                textFormat: Text.PlainText
                elide: Text.ElideRight
            }
            CompactTextField {
                Layout.preferredWidth: 155
                text: entry.modelData.stackSize ? String(entry.modelData.stackSize) : ""
                placeholderText: qsTrId("monitor.stackOptional")
                Accessible.name: qsTrId("monitor.stackSize")
                validator: IntValidator { bottom: 1; top: 2147483647; locale: "C" }
                onEditingFinished: {
                    if (text.length && !acceptableInput)
                        return;
                    const size = Number(text);
                    if (size !== (entry.modelData.stackSize || 0))
                        entry.editor.edited(entry.editor.items.map((item, index) => index === entry.index
                            ? ({id: item.id, source: item.source, path: item.path, stackSize: size}) : item));
                }
            }
            ToolButton {
                text: "×"
                Accessible.name: qsTrId("catalog.item.remove").arg(entry.modelData.id)
                onClicked: entry.editor.edited(entry.editor.items.filter((item, index) => index !== entry.index))
            }
        }
    }
}
