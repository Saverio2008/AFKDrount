import QtQuick
import QtQuick.Controls

Dialog {
    id: dialog
    property string sessionId: ""
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 48, 520)
    title: qsTrId("session.apply.title")
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    onAccepted: SessionStore.applyConfiguration(dialog.sessionId)
    contentItem: Label {
        text: qsTrId("session.apply.hint")
        wrapMode: Text.WordWrap
    }
}