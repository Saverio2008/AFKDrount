import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog

    //noinspection JSUnusedGlobalSymbols
    signal instanceCreated(string instanceId)
    property string gameVersion: ""

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 48, 480)
    title: qsTrId("instance.create.title")
    modal: true
    closePolicy: Popup.CloseOnEscape

    onOpened: {
        newName.clear();
        newServer.clear();
        newPort.text = "25565";
        dialog.gameVersion = "";
        newName.forceActiveFocus();
    }

    ColumnLayout {
        width: parent.width
        spacing: 12

        Label {
            text: qsTrId("instance.name")
        }
        TextField {
            id: newName

            Layout.fillWidth: true
            maximumLength: 80
            selectByMouse: true
            placeholderText: qsTrId("instance.name.placeholder")
            Accessible.name: qsTrId("instance.create.name.accessible")
        }
        Label {
            text: qsTrId("instance.server")
        }
        TextField {
            id: newServer

            Layout.fillWidth: true
            selectByMouse: true
            placeholderText: qsTrId("instance.server.placeholder")
            Accessible.name: qsTrId("instance.create.server.accessible")
        }
        Label {
            text: qsTrId("instance.port")
        }
        TextField {
            id: newPort

            Layout.fillWidth: true
            selectByMouse: true
            placeholderText: qsTrId("input.example.number").arg(25565)
            validator: IntValidator {
                bottom: 1
                top: 65535
                locale: "C"
            }
            Accessible.name: qsTrId("instance.create.port.accessible")
        }
        Label {
            text: qsTrId("config.version")
        }
        VersionSelector {
            Layout.fillWidth: true
            version: dialog.gameVersion
            onSelected: value => dialog.gameVersion = value
        }
        Label {
            Layout.fillWidth: true
            visible: InstanceStore.error.length > 0
            text: InstanceStore.error
            wrapMode: Text.WordWrap
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTrId("action.cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            text: qsTrId("action.create")
            enabled: newName.text.trim().length > 0 && newServer.text.trim().length > 0 && !(/[\s/\\]/.test(newServer.text)) && newPort.acceptableInput && dialog.gameVersion.length > 0
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                const instanceId = InstanceStore.createInstance(newName.text, newServer.text, Number(newPort.text), dialog.gameVersion);
                if (instanceId.length > 0) {
                    dialog.instanceCreated(instanceId);
                    dialog.close();
                }
            }
        }
    }
}
