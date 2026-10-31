import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    property var configuration: InstanceStore.defaultConfiguration()
    //noinspection JSUnusedGlobalSymbols
    signal edited(var value)

    function update(key, value) {
        const copy = JSON.parse(JSON.stringify(root.configuration));
        copy[key] = value;
        root.edited(copy);
    }

    function updateNested(group, key, value) {
        const copy = JSON.parse(JSON.stringify(root.configuration));
        copy[group][key] = value;
        if (group === "reconnect" && key === "unlimited" && value && !(Number.isInteger(copy.reconnect.maxAttempts) && copy.reconnect.maxAttempts >= 1 && copy.reconnect.maxAttempts <= 1000000))
            copy.reconnect.maxAttempts = 5;
        root.edited(copy);
    }

    spacing: 12

    TasksEditor {
        Layout.fillWidth: true
        gameVersion: root.configuration.gameVersion
        tasks: root.configuration.tasks || []
        onEdited: value => root.update("tasks", value)
    }

    CountingRulesEditor {
        Layout.fillWidth: true
        gameVersion: root.configuration.gameVersion
        rules: root.configuration.rules
        onEdited: value => root.update("rules", value)
    }
    Label {
        text: qsTrId("config.failure.title")
        font.weight: Font.Medium
    }
    FailurePolicyRow {
        Layout.fillWidth: true
        title: qsTrId("config.failure.disconnect")
        policy: root.configuration.failurePolicies.disconnect
        onSelected: value => root.updateNested("failurePolicies", "disconnect", value)
    }
    FailurePolicyRow {
        Layout.fillWidth: true
        title: qsTrId("config.failure.kicked")
        policy: root.configuration.failurePolicies.kicked
        onSelected: value => root.updateNested("failurePolicies", "kicked", value)
    }
    FailurePolicyRow {
        Layout.fillWidth: true
        title: qsTrId("config.failure.connectionError")
        policy: root.configuration.failurePolicies.connectionError
        onSelected: value => root.updateNested("failurePolicies", "connectionError", value)
    }
    FailurePolicyRow {
        Layout.fillWidth: true
        title: qsTrId("config.failure.hurt")
        policy: root.configuration.failurePolicies.hurt
        movement: true
        onSelected: value => root.updateNested("failurePolicies", "hurt", value)
    }
    FailurePolicyRow {
        Layout.fillWidth: true
        title: qsTrId("config.failure.movement")
        policy: root.configuration.failurePolicies.unexpectedMovement
        movement: true
        onSelected: value => root.updateNested("failurePolicies", "unexpectedMovement", value)
    }
    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: 12
        rowSpacing: 8

        Label {
            text: qsTrId("config.reconnect.delay")
        }
        CompactTextField {
            Layout.preferredWidth: 140
            text: root.configuration.reconnect.delaySeconds === null ? "" : String(root.configuration.reconnect.delaySeconds)
            placeholderText: delayRange.bottom + "–" + delayRange.top
            validator: IntValidator {
                id: delayRange
                bottom: 1
                top: 86400
                locale: "C"
            }
            Accessible.name: qsTrId("config.reconnect.delay")
            onTextEdited: root.updateNested("reconnect", "delaySeconds", text.length > 0 ? Number(text) : null)
        }
        Label {
            text: qsTrId("config.reconnect.attempts")
        }
        CompactTextField {
            Layout.preferredWidth: 140
            enabled: !root.configuration.reconnect.unlimited
            text: root.configuration.reconnect.maxAttempts === null ? "" : String(root.configuration.reconnect.maxAttempts)
            placeholderText: enabled ? attemptsRange.bottom + "–" + attemptsRange.top : ""
            validator: IntValidator {
                id: attemptsRange
                bottom: 1
                top: 1000000
                locale: "C"
            }
            Accessible.name: qsTrId("config.reconnect.attempts")
            onTextEdited: root.updateNested("reconnect", "maxAttempts", text.length > 0 ? Number(text) : null)
        }
        CheckBox {
            Layout.columnSpan: 2
            text: qsTrId("config.reconnect.unlimited")
            checked: root.configuration.reconnect.unlimited
            onClicked: root.updateNested("reconnect", "unlimited", checked)
        }
    }
}
