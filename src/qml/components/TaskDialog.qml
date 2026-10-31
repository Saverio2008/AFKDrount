import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Rounded
import AppAppearance

Dialog {
    id: dialog
    property var task: InstanceStore.newTask()
    property string gameVersion: ""
    property var draftSteps: []
    readonly property int stepCount: steps.count
    readonly property var draft: ({
            id: task.id,
            name: nameField.text.trim(),
            enabled: enabledField.checked,
            trigger: ["startup", "connection", "world"][triggerField.currentIndex],
            steps: dialog.draftSteps
        })
    readonly property string errorId: InstanceStore.taskError(draft, gameVersion)
    //noinspection JSUnusedGlobalSymbols
    signal taskAccepted(var value)

    function updateStep(index, key, value) {
        steps.setProperty(index, key, value);
        dialog.refreshSteps();
    }
    function moveStep(index, delta) {
        steps.move(index, index + delta, 1);
        dialog.refreshSteps();
    }
    function removeStep(index) {
        steps.remove(index);
        dialog.refreshSteps();
    }
    function addStep() {
        steps.append({
            mode: "chat",
            content: "",
            waitSeconds: 1.0
        });
        dialog.refreshSteps();
    }
    //noinspection JSUnusedGlobalSymbols
    function refreshSteps() {
        dialog.draftSteps = Array.from({
            length: steps.count
        }, (_, index) => ({
                    mode: steps.get(index).mode,
                    text: steps.get(index).content,
                    delaySeconds: steps.get(index).waitSeconds
                }));
    }

    ListModel {
        id: steps
    }
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 48, 680)
    height: Math.min(parent.height - 48, implicitHeight)
    rightPadding: 6
    title: qsTrId("task.edit")
    modal: true
    closePolicy: Popup.CloseOnEscape
    onOpened: {
        nameField.text = task.name;
        enabledField.checked = task.enabled;
        triggerField.currentIndex = ["startup", "connection", "world"].indexOf(task.trigger);
        steps.clear();
        for (const step of task.steps)
            steps.append({
                mode: step.mode,
                content: step.text,
                waitSeconds: step.delaySeconds
            });
        dialog.refreshSteps();
        nameField.forceActiveFocus();
    }
    contentItem: ScrollView {
        id: scroll
        readonly property var scrollableItem: contentItem
        Component.onCompleted: scrollableItem.boundsBehavior = Flickable.StopAtBounds
        rightPadding: Math.max(dialog.leftPadding - dialog.rightPadding, effectiveScrollBarWidth + 8)
        contentWidth: availableWidth
        implicitHeight: form.implicitHeight
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ColumnLayout {
            id: form
            property var taskDialog: dialog
            width: scroll.availableWidth
            spacing: 12
            RowLayout {
                Layout.fillWidth: true
                CompactTextField {
                    id: nameField
                    Layout.fillWidth: true
                    maximumLength: 80
                    placeholderText: qsTrId("task.name")
                    Accessible.name: qsTrId("task.name")
                }
                CheckBox {
                    id: enabledField
                    text: qsTrId("config.enabled")
                }
            }
            Label {
                text: qsTrId("task.trigger")
            }
            ComboBox {
                id: triggerField
                Layout.fillWidth: true
                readonly property var scrollablePopup: popup.contentItem
                Component.onCompleted: scrollablePopup.boundsBehavior = Flickable.StopAtBounds
                textRole: "label"
                model: ListModel {
                    ListElement {
                        label: qsTrId("task.trigger.startup")
                    }
                    ListElement {
                        label: qsTrId("task.trigger.connection")
                    }
                    ListElement {
                        label: qsTrId("task.trigger.world")
                    }
                }
                Accessible.name: qsTrId("task.trigger")
            }
            Repeater {
                model: steps
                delegate: TaskStepEditor {
                    readonly property var owner: parent
                    Layout.fillWidth: true
                    stepCount: owner.taskDialog.stepCount
                    onEdited: (key, value) => owner.taskDialog.updateStep(index, key, value)
                    onMoveRequested: delta => owner.taskDialog.moveStep(index, delta)
                    onRemoveRequested: owner.taskDialog.removeStep(index)
                }
            }
            ToolButton {
                text: qsTrId("task.step.add")
                enabled: steps.count < 64
                onClicked: dialog.addStep()
            }
            Label {
                Layout.fillWidth: true
                visible: !GameCatalog.supportsCommands(dialog.gameVersion)
                text: qsTrId("task.error.commandVersion")
                wrapMode: Text.WordWrap
                color: Theme.muted
            }
            Label {
                Layout.fillWidth: true
                text: dialog.errorId.length > 0 && dialog.errorId !== "task.error.format" ? qsTrId(dialog.errorId) : ""
                visible: text.length > 0
                wrapMode: Text.WordWrap
                Accessible.role: Accessible.AlertMessage
            }
        }
    }
    footer: DialogButtonBox {
        Button {
            text: qsTrId("action.cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            text: qsTrId("action.save")
            enabled: dialog.errorId.length === 0
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                dialog.taskAccepted(dialog.draft);
                dialog.close();
            }
        }
    }
}
