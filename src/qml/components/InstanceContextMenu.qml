import QtQuick
import QtQuick.Controls

Menu {
    id: contextMenu
    readonly property var scrollableItem: contentItem
    Component.onCompleted: scrollableItem.boundsBehavior = Flickable.StopAtBounds

    property var record: null
    readonly property bool available: record !== null && InstanceStore.instances.some(instance => instance.id === contextMenu.record.id)

    signal createRequested
    signal editRequested(string instanceId)
    signal duplicateRequested(string instanceId)
    signal deleteRequested(string instanceId)

    MenuItem {
        height: visible ? implicitHeight : 0
        text: qsTrId("instance.action.new")
        visible: contextMenu.record === null
        onTriggered: contextMenu.createRequested()
    }
    MenuItem {
        height: visible ? implicitHeight : 0
        text: qsTrId("action.edit")
        visible: contextMenu.record !== null
        enabled: contextMenu.available
        onTriggered: contextMenu.editRequested(contextMenu.record.id)
    }
    MenuItem {
        height: visible ? implicitHeight : 0
        text: qsTrId("instance.action.duplicate")
        visible: contextMenu.record !== null
        enabled: contextMenu.available
        onTriggered: contextMenu.duplicateRequested(contextMenu.record.id)
    }
    MenuItem {
        height: visible ? implicitHeight : 0
        text: qsTrId("instance.action.copyServer")
        visible: contextMenu.record !== null
        enabled: contextMenu.available
        onTriggered: {
            const server = contextMenu.record.server;
            const host = server.includes(":") && !server.startsWith("[") ? "[" + server + "]" : server;
            Clipboard.copyText("%1:%2".arg(host).arg(contextMenu.record.port));
        }
    }
    MenuSeparator {
        height: visible ? implicitHeight : 0
        visible: contextMenu.record !== null
    }
    MenuItem {
        height: visible ? implicitHeight : 0
        text: qsTrId("action.delete")
        visible: contextMenu.record !== null
        enabled: contextMenu.available
        onTriggered: contextMenu.deleteRequested(contextMenu.record.id)
    }
}