import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

ListView {
    id: root
    boundsBehavior: Flickable.StopAtBounds

    property string contextSessionId: ""
    property string deletingSessionId: ""
    readonly property var contextRecord: SessionStore.sessions.find(session => session.id === root.contextSessionId) || null

    function showContext(sessionId: string, anchor: Item, popupX: real, popupY: real) {
        root.contextSessionId = sessionId;
        recordMenu.popup(anchor, popupX, popupY, null);
    }

    Menu {
        id: recordMenu
        readonly property var scrollableItem: contentItem
        Component.onCompleted: scrollableItem.boundsBehavior = Flickable.StopAtBounds
        MenuItem {
            text: qsTrId("session.record.delete")
            enabled: root.contextRecord !== null && !root.contextRecord.active
            onTriggered: {
                root.deletingSessionId = root.contextSessionId;
                deleteDialog.open();
            }
        }
    }

    Dialog {
        id: deleteDialog
        readonly property var record: SessionStore.sessions.find(session => session.id === root.deletingSessionId) || null
        property string failure: ""
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 520)
        title: qsTrId("session.record.delete")
        modal: true
        closePolicy: Popup.CloseOnEscape
        onOpened: failure = ""
        ColumnLayout {
            width: parent.width
            Label {
                Layout.fillWidth: true
                text: deleteDialog.record ? qsTrId("session.record.deleteHint").arg(deleteDialog.record.accountName || "—").arg(deleteDialog.record.instanceName).arg(deleteDialog.record.startedAt) : qsTrId("session.error.notFound")
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
            }
            Label {
                Layout.fillWidth: true
                visible: deleteDialog.failure.length > 0
                text: deleteDialog.failure.length > 0 ? qsTrId(deleteDialog.failure) : ""
                wrapMode: Text.WordWrap
                Accessible.role: Accessible.AlertMessage
            }
        }
        footer: DialogButtonBox {
            Button {
                text: qsTrId("action.cancel")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
            Button {
                text: qsTrId("action.delete")
                enabled: deleteDialog.record !== null && !deleteDialog.record.active
                DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
                onClicked: {
                    if (SessionStore.removeRecord(root.deletingSessionId))
                        deleteDialog.accept();
                    else
                        deleteDialog.failure = SessionStore.errorCode;
                }
            }
        }
    }
    signal stopRequested(string sessionId)
    signal monitorRequested(string sessionId)
    signal applyRequested(string sessionId)

    spacing: 8
    clip: true
    ScrollBar.vertical: ScrollBar {}

    delegate: Rectangle {
        id: row

        required property var modelData
        readonly property var owner: ListView.view

        width: owner.width
        height: 64
        radius: Theme.rowRadius
        color: Theme.field
        ContextMenu.onRequested: position => row.owner.showContext(row.modelData.id, row, position.x, position.y)

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            Label {
                text: row.modelData.accountName || "—"
            }
            Label {
                Layout.fillWidth: true
                text: row.modelData.instanceName
                elide: Text.ElideRight
            }
            Label {
                text: qsTrId("session.state." + row.modelData.state)
                color: row.modelData.state === "running" ? Theme.running : row.modelData.state === "waiting" ? Theme.queued : Theme.muted
            }
            Button {
                text: qsTrId("action.stop")
                visible: row.modelData.active
                enabled: row.modelData.state !== "stopping"
                onClicked: row.owner.stopRequested(row.modelData.id)
            }
            ToolButton {
                text: qsTrId("session.apply.action")
                visible: row.modelData.active
                enabled: row.modelData.state !== "stopping" && !row.modelData.applying
                onClicked: row.owner.applyRequested(row.modelData.id)
            }
            ToolButton {
                text: qsTrId("action.monitor")
                onClicked: row.owner.monitorRequested(row.modelData.id)
            }
        }
    }
}