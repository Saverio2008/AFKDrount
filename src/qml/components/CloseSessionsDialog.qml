import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog
    signal closeConfirmed
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 48, 480)
    title: qsTrId("session.close.title")
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel
    onOpened: remember.checked = false
    onAccepted: {
        if (remember.checked)
            SessionStore.confirmClose = false;
        dialog.closeConfirmed();
    }
    ColumnLayout {
        width: parent.width
        Label {
            Layout.fillWidth: true
            text: SessionStore.stopOnClose ? qsTrId("session.close.stopHint") : qsTrId("session.close.keepHint")
            wrapMode: Text.WordWrap
        }
        CheckBox {
            id: remember
            text: qsTrId("session.close.remember")
        }
    }
}