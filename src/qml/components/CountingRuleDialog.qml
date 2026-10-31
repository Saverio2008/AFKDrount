import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Rounded
import AppAppearance

Dialog {
    id: dialog

    property var rule: InstanceStore.newCountingRule()
    property string gameVersion: ""
    property var selectedItems: []
    property var customItems: []
    readonly property var draft: ({
            id: rule.id,
            name: nameField.text.trim(),
            enabled: enabledField.checked,
            items: dialog.selectedItems,
            customItems: dialog.customItems,
            trackingRegion: tracking.draft,
            countingRegion: counting.draft
        })
    readonly property string errorId: InstanceStore.countingRuleError(draft, gameVersion)
    //noinspection JSUnusedGlobalSymbols
    signal ruleAccepted(var value)

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 48, 680)
    height: Math.min(parent.height - 48, implicitHeight)
    rightPadding: 6
    title: qsTrId("config.rule.edit")
    modal: true
    closePolicy: Popup.CloseOnEscape
    onOpened: {
        nameField.text = rule.name;
        dialog.selectedItems = rule.items.slice();
        dialog.customItems = JSON.parse(JSON.stringify(rule.customItems || []));
        enabledField.checked = rule.enabled;
        nameField.forceActiveFocus();
    }

    contentItem: ScrollView {
        id: scroll
        readonly property var scrollableItem: contentItem
        Component.onCompleted: scrollableItem.boundsBehavior = Flickable.StopAtBounds
        rightPadding: Math.max(dialog.leftPadding - dialog.rightPadding, effectiveScrollBarWidth + 8)
        contentWidth: availableWidth
        clip: true
        implicitHeight: form.implicitHeight
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            id: form
            width: scroll.availableWidth
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                CompactTextField {
                    id: nameField
                    Layout.fillWidth: true
                    maximumLength: 80
                    placeholderText: qsTrId("config.rule.name")
                    Accessible.name: qsTrId("config.rule.name")
                }
                CheckBox {
                    id: enabledField
                    text: qsTrId("config.enabled")
                }
            }
            Label {
                text: qsTrId("config.rule.items")
            }
            ItemPicker {
                Layout.fillWidth: true
                gameVersion: dialog.gameVersion
                selectedItems: dialog.selectedItems
                onEdited: value => dialog.selectedItems = value
            }
            CustomItemEditor {
                Layout.fillWidth: true
                items: dialog.customItems
                onEdited: value => dialog.customItems = value
            }
            RegionEditor {
                id: tracking
                Layout.fillWidth: true
                title: qsTrId("config.region.tracking")
                region: dialog.rule.trackingRegion
                allowUnlimitedY: true
            }
            RegionEditor {
                id: counting
                Layout.fillWidth: true
                title: qsTrId("config.region.counting")
                region: dialog.rule.countingRegion
            }
            Label {
                Layout.fillWidth: true
                text: qsTrId("config.region.hint")
                color: Theme.muted
                wrapMode: Text.WordWrap
            }
            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: dialog.errorId.length > 0 && dialog.errorId !== "config.error.ruleName" ? qsTrId(dialog.errorId) : ""
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
                dialog.ruleAccepted(dialog.draft);
                dialog.close();
            }
        }
    }
}
