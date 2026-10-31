import QtQuick
import QtQuick.Controls

Menu {
    id: contextMenu
    readonly property var scrollableItem: contentItem
    Component.onCompleted: scrollableItem.boundsBehavior = Flickable.StopAtBounds

    property var record: null
    readonly property bool available: record !== null && AccountStore.accounts.some(account => account.id === contextMenu.record.id)

    signal addRequested
    signal loginRequested(string accountId)
    signal deleteRequested(string accountId)

    MenuItem {
        height: visible ? implicitHeight : 0
        text: qsTrId("account.action.add")
        visible: contextMenu.record === null
        enabled: !AccountStore.busy
        onTriggered: contextMenu.addRequested()
    }
    MenuItem {
        height: visible ? implicitHeight : 0
        text: qsTrId("account.login.again")
        visible: contextMenu.record !== null && contextMenu.record.type === "microsoft"
        enabled: contextMenu.available && !AccountStore.busy
        onTriggered: contextMenu.loginRequested(contextMenu.record.id)
    }
    MenuItem {
        height: visible ? implicitHeight : 0
        text: qsTrId("action.copyName")
        visible: contextMenu.record !== null
        enabled: contextMenu.available
        onTriggered: Clipboard.copyText(contextMenu.record.name)
    }
    MenuItem {
        height: visible ? implicitHeight : 0
        text: qsTrId("action.copyUuid")
        visible: contextMenu.record !== null
        enabled: contextMenu.available
        onTriggered: Clipboard.copyText(contextMenu.record.uuid)
    }
    MenuSeparator {
        height: visible ? implicitHeight : 0
        visible: contextMenu.record !== null
    }
    MenuItem {
        height: visible ? implicitHeight : 0
        text: qsTrId("action.delete")
        visible: contextMenu.record !== null
        enabled: contextMenu.available && !AccountStore.busy
        onTriggered: contextMenu.deleteRequested(contextMenu.record.id)
    }
}