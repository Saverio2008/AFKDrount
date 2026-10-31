import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import AppAppearance

Page {
    id: page

    padding: Theme.gutter

    property var selectedInstance: null
    property var selectedAccount: null
    property var pendingAction: null
    property bool closeApproved: false
    property bool selectionReady: false

    Settings {
        id: selection
        category: "selection"
        property bool initialized: false
        property string accountId: ""
        property string instanceId: ""
    }

    onSelectedAccountChanged: {
        if (page.selectionReady)
            selection.accountId = page.selectedAccount ? page.selectedAccount.id : "";
    }
    onSelectedInstanceChanged: {
        if (page.selectionReady)
            selection.instanceId = page.selectedInstance ? page.selectedInstance.id : "";
    }

    signal settingsRequested
    signal windowCloseRequested
    signal monitorRequested(string sessionId)

    function findInstance(instanceId) {
        for (const instance of InstanceStore.instances) {
            if (instance.id === instanceId)
                return instance;
        }
        return null;
    }

    function saveInstance() {
        if (!selectedInstance || !editor.valid)
            return false;
        if (!InstanceStore.updateInstance(selectedInstance.id, editor.draftName, editor.draftServer, editor.draftPort, editor.draftConfiguration))
            return false;
        page.selectedInstance = page.findInstance(page.selectedInstance.id);
        return true;
    }

    function afterConfirmation(action) {
        if (editor.dirty) {
            page.pendingAction = action;
            unsavedDialog.open();
        } else {
            action();
        }
    }

    //noinspection JSUnusedGlobalSymbols
    function continuePending() {
        const action = page.pendingAction;
        page.pendingAction = null;
        unsavedDialog.close();
        if (action)
            action();
    }

    function requestClose() {
        if (editor.dirty) {
            afterConfirmation(() => page.windowCloseRequested());
            return false;
        }
        if (SessionStore.activeCount > 0 && SessionStore.confirmClose && !page.closeApproved) {
            closeSessionsDialog.open();
            return false;
        }
        page.closeApproved = false;
        return true;
    }

    function editInstance(instanceId) {
        if (page.selectedInstance && page.selectedInstance.id === instanceId) {
            editor.focusName();
            return;
        }
        page.afterConfirmation(() => {
            page.selectedInstance = page.findInstance(instanceId);
            editor.focusName();
        });
    }

    function duplicateInstance(instanceId) {
        page.afterConfirmation(() => {
            const copyId = InstanceStore.duplicateInstance(instanceId);
            if (copyId.length > 0)
                page.selectedInstance = page.findInstance(copyId);
        });
    }

    function requestDelete(entryId, account) {
        const record = account ? AccountStore.accounts.find(entry => entry.id === entryId) : page.findInstance(entryId);
        if (!record)
            return;
        deleteDialog.targetId = record.id;
        deleteDialog.targetName = record.name;
        deleteDialog.account = account;
        deleteDialog.removeCredentials = account && record.type === "microsoft";
        deleteDialog.open();
    }

    Component.onCompleted: {
        page.selectedInstance = selection.initialized ? page.findInstance(selection.instanceId) : InstanceStore.instances[0] || null;
        page.selectedAccount = AccountStore.accounts.find(account => account.id === selection.accountId) || null;
        page.selectionReady = true;
        selection.instanceId = page.selectedInstance ? page.selectedInstance.id : "";
        selection.initialized = true;
    }

    header: ToolBar {
        padding: 16

        RowLayout {
            anchors.fill: parent
            spacing: 12

            Label {
                Layout.fillWidth: true
                text: "AFK Drount"
                font.pointSize: 18
                font.weight: Font.Medium
            }

            ToolButton {
                text: qsTrId("settings.title")
                onClicked: page.settingsRequested()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            Layout.fillWidth: true
            Layout.margins: 16
            visible: InstanceStore.error.length > 0
            text: InstanceStore.error
            wrapMode: Text.WordWrap
            Accessible.role: Accessible.AlertMessage
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            SplitView {
                SplitView.preferredWidth: 280
                SplitView.minimumWidth: 240
                SplitView.maximumWidth: 380
                orientation: Qt.Vertical

                AccountPanel {
                    id: accountPanel

                    SplitView.preferredHeight: 260
                    SplitView.minimumHeight: 170
                    accounts: AccountStore.accounts
                    selectedId: page.selectedAccount ? page.selectedAccount.id : ""
                    onAccountSelected: accountId => {
                        for (const account of accountPanel.accounts) {
                            if (account.id === accountId) {
                                page.selectedAccount = account;
                                break;
                            }
                        }
                    }
                    onAddRequested: {
                        accountDialog.accountId = "";
                        accountDialog.open();
                    }
                    onDeleteRequested: entryId => page.requestDelete(entryId, true)
                    onLoginRequested: accountId => {
                        accountDialog.accountId = accountId;
                        accountDialog.open();
                    }
                }

                InstancePanel {
                    id: instancePanel

                    SplitView.fillHeight: true
                    SplitView.minimumHeight: 190
                    instances: InstanceStore.instances
                    selectedId: page.selectedInstance ? page.selectedInstance.id : ""
                    onInstanceSelected: instanceId => {
                        if (instanceId !== instancePanel.selectedId)
                            page.afterConfirmation(() => {
                                page.selectedInstance = page.findInstance(instanceId);
                            });
                    }
                    onCreateRequested: page.afterConfirmation(() => newInstanceDialog.open())
                    onEditRequested: instanceId => page.editInstance(instanceId)
                    onDuplicateRequested: instanceId => page.duplicateInstance(instanceId)
                    onDeleteRequested: entryId => page.requestDelete(entryId, false)
                }
            }

            SplitView {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 610
                orientation: Qt.Vertical

                InstanceEditor {
                    id: editor

                    SplitView.fillHeight: true
                    SplitView.minimumHeight: 330
                    record: page.selectedInstance
                    onSaveRequested: page.saveInstance()
                    onCreateRequested: page.afterConfirmation(() => newInstanceDialog.open())
                }

                RunPanel {
                    SplitView.preferredHeight: 220
                    SplitView.minimumHeight: 170
                    accountName: page.selectedAccount ? page.selectedAccount.name : ""
                    instanceName: page.selectedInstance ? page.selectedInstance.name : ""
                    hasUnsavedChanges: editor.dirty
                    configurationReady: page.selectedInstance !== null && InstanceStore.configurationError(page.selectedInstance.configuration).length === 0
                    sessions: SessionStore.sessions
                    onRunRequested: page.afterConfirmation(() => {
                        if (page.selectedAccount && page.selectedInstance)
                            SessionStore.startSession(page.selectedAccount.id, page.selectedInstance.id);
                    })
                    onStopRequested: sessionId => SessionStore.stopSession(sessionId)
                    onMonitorRequested: sessionId => page.monitorRequested(sessionId)
                    onApplyRequested: sessionId => page.afterConfirmation(() => {
                            applyDialog.sessionId = sessionId;
                            applyDialog.open();
                        })
                }
            }
        }
    }

    ApplySessionConfigurationDialog {
        id: applyDialog
    }
    CloseSessionsDialog {
        id: closeSessionsDialog
        onCloseConfirmed: {
            page.closeApproved = true;
            page.windowCloseRequested();
        }
    }

    AccountDialog {
        id: accountDialog
    }

    Connections {
        target: AccountStore

        //noinspection JSUnusedGlobalSymbols
        function onAccountsChanged() {
            if (!page.selectionReady)
                return;
            const id = page.selectedAccount ? page.selectedAccount.id : selection.accountId;
            page.selectedAccount = AccountStore.accounts.find(account => account.id === id) || null;
            selection.accountId = page.selectedAccount ? page.selectedAccount.id : "";
        }

        //noinspection JSUnusedGlobalSymbols
        function onAccountRemoved(accountId) {
            if (deleteDialog.opened && deleteDialog.account && deleteDialog.targetId === accountId)
                deleteDialog.accept();
        }

        //noinspection JSUnusedGlobalSymbols
        function onStateChanged() {
            if (deleteDialog.opened && deleteDialog.account && deleteDialog.submitted && !AccountStore.busy)
                deleteDialog.errorText = AccountStore.errorCode.length > 0 ? qsTrId(AccountStore.errorCode) : "";
        }

        //noinspection JSUnusedGlobalSymbols
        function onAccountSaved(accountId) {
            page.selectedAccount = AccountStore.accounts.find(account => account.id === accountId) || null;
        }
    }

    Connections {
        target: InstanceStore

        //noinspection JSUnusedGlobalSymbols
        function onInstancesChanged() {
            if (page.selectionReady && page.selectedInstance && !page.findInstance(page.selectedInstance.id))
                page.selectedInstance = null;
        }
    }

    DeleteConfirmationDialog {
        id: deleteDialog

        busy: account && AccountStore.busy
        hasUnsavedChanges: !account && page.selectedInstance !== null && page.selectedInstance.id === targetId && editor.dirty
        onDeleteConfirmed: {
            deleteDialog.errorText = "";
            if (deleteDialog.account) {
                AccountStore.removeAccount(deleteDialog.targetId);
            } else if (InstanceStore.removeInstance(deleteDialog.targetId)) {
                if (page.selectedInstance && page.selectedInstance.id === deleteDialog.targetId)
                    page.selectedInstance = null;
                deleteDialog.accept();
            } else {
                deleteDialog.errorText = InstanceStore.error;
            }
        }
    }

    CreateInstanceDialog {
        id: newInstanceDialog

        onInstanceCreated: instanceId => {
            page.selectedInstance = page.findInstance(instanceId);
        }
    }

    UnsavedChangesDialog {
        id: unsavedDialog

        canSave: editor.valid
        errorText: InstanceStore.error
        onRejected: page.pendingAction = null
        onSaveRequested: {
            if (page.saveInstance())
                page.continuePending();
        }
        onDiscardRequested: {
            editor.resetDraft();
            page.continuePending();
        }
    }
}
