import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog

    property string targetId: ""
    property string targetName: ""
    property bool account: false
    property bool removeCredentials: false
    property bool hasUnsavedChanges: false
    property bool busy: false
    property bool submitted: false
    property string errorText: ""
    readonly property int runningSessionCount: SessionStore.sessions.filter(session => session.active && (dialog.account ? session.accountId : session.instanceId) === dialog.targetId).length

    signal deleteConfirmed

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 48, 520)
    title: account ? qsTrId("delete.account.title") : qsTrId("delete.instance.title")
    modal: true
    closePolicy: busy ? Popup.NoAutoClose : Popup.CloseOnEscape
    onOpened: {
        dialog.errorText = "";
        dialog.submitted = false;
    }

    ColumnLayout {
        width: parent.width
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: dialog.account ? qsTrId("delete.account.description").arg(dialog.targetName) : qsTrId("delete.instance.description").arg(dialog.targetName)
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            visible: dialog.removeCredentials
            text: qsTrId("delete.account.credentialsHint")
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            visible: dialog.runningSessionCount > 0
            text: dialog.account ? qsTrId("delete.account.runningHint").arg(dialog.runningSessionCount) : qsTrId("delete.instance.runningHint").arg(dialog.runningSessionCount)
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            visible: dialog.hasUnsavedChanges
            text: qsTrId("delete.instance.unsavedHint")
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            visible: dialog.errorText.length > 0
            text: dialog.errorText
            wrapMode: Text.WordWrap
            Accessible.role: Accessible.AlertMessage
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTrId("action.cancel")
            enabled: !dialog.busy
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            text: qsTrId("action.delete")
            enabled: !dialog.busy
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                dialog.submitted = true;
                dialog.deleteConfirmed();
            }
        }
    }
}