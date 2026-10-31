import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

Pane {
    id: root

    palette.window: Theme.instances
    palette.button: Theme.mint
    palette.buttonText: Theme.mintText
    palette.highlight: Theme.mintSelection

    required property var instances
    property string selectedId: ""

    signal instanceSelected(string instanceId)
    signal createRequested

    signal deleteRequested(string entryId)
    signal editRequested(string instanceId)
    signal duplicateRequested(string instanceId)

    function showMenu(entryId: string, anchor: Item, popupX: real, popupY: real) {
        entryMenu.record = root.instances.find(entry => entry.id === entryId) || null;
        entryMenu.popup(anchor, popupX, popupY, null);
    }

    InstanceContextMenu {
        id: entryMenu

        onDeleteRequested: entryId => root.deleteRequested(entryId)
        onCreateRequested: root.createRequested()
        onEditRequested: instanceId => root.editRequested(instanceId)
        onDuplicateRequested: instanceId => root.duplicateRequested(instanceId)
    }

    padding: 12

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                Layout.fillWidth: true
                text: qsTrId("instance.list.title")
                font.weight: Font.Medium
                elide: Text.ElideRight
            }

            ToolButton {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                padding: 4
                horizontalPadding: 4
                text: "＋"
                Accessible.name: qsTrId("instance.action.new")
                ToolTip.visible: hovered
                ToolTip.text: qsTrId("instance.action.new")
                onClicked: root.createRequested()
            }
            ToolButton {
                id: moreButton

                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                padding: 4
                horizontalPadding: 4
                text: "⋯"
                Accessible.name: qsTrId("action.more")
                ToolTip.visible: hovered
                ToolTip.text: qsTrId("action.more")
                enabled: root.selectedId.length > 0
                onClicked: root.showMenu(root.selectedId, moreButton, 0, moreButton.height)
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            EmptyState {
                anchors.centerIn: parent
                width: parent.width
                visible: root.instances.length === 0
                title: qsTrId("instance.empty.create")
                description: qsTrId("instance.list.emptyDescription")
            }

            SelectionList {
                anchors.fill: parent
                entries: root.instances
                selectedId: root.selectedId
                showServer: true
                moveEntry: (entryId, targetId, after) => InstanceStore.moveInstance(entryId, targetId, after)
                onEntryActivated: entryId => root.instanceSelected(entryId)
                onContextRequested: (entryId, anchor, position) => root.showMenu(entryId, anchor, position.x, position.y)
                onEmptyContextRequested: (anchor, position) => root.showMenu("", anchor, position.x, position.y)
            }
        }
    }
}