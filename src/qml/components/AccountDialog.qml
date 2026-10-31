import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog

    property string accountId: ""
    property bool microsoft: false
    readonly property bool validOfflineName: /^[A-Za-z0-9_]{1,16}$/.test(offlineName.text)

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 48, 580)
    title: accountId.length > 0 ? qsTrId("account.login.again") : qsTrId("account.add.title")
    modal: true
    closePolicy: AccountStore.busy ? Popup.NoAutoClose : Popup.CloseOnEscape

    onOpened: {
        dialog.microsoft = dialog.accountId.length > 0;
        offlineName.clear();
    }
    onClosed: {
        if (AccountStore.busy)
            AccountStore.cancel();
    }

    Connections {
        target: AccountStore

        //noinspection JSUnusedGlobalSymbols
        function onAccountSaved(_accountId) {
            if (dialog.opened)
                dialog.accept();
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: 16

        RowLayout {
            visible: dialog.accountId.length === 0
            enabled: !AccountStore.busy

            ToolButton {
                text: qsTrId("account.type.offline")
                checkable: true
                checked: !dialog.microsoft
                onClicked: dialog.microsoft = false
            }
            ToolButton {
                text: qsTrId("account.type.microsoft")
                checkable: true
                checked: dialog.microsoft
                onClicked: dialog.microsoft = true
            }
        }
        TextField {
            id: offlineName

            Layout.fillWidth: true
            visible: !dialog.microsoft
            enabled: !AccountStore.busy
            maximumLength: 16
            selectByMouse: true
            placeholderText: qsTrId("account.offline.nameHint")
            Accessible.name: qsTrId("account.offline.name")
        }
        Label {
            Layout.fillWidth: true
            visible: dialog.microsoft
            text: qsTrId("account.login.description")
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            visible: AccountStore.busy
            text: AccountStore.deviceCode.length > 0 ? qsTrId("account.login.codeHint") : qsTrId("account.login.waiting")
            wrapMode: Text.WordWrap
        }
        TextField {
            Layout.fillWidth: true
            visible: AccountStore.deviceCode.length > 0
            readOnly: true
            selectByMouse: true
            text: AccountStore.deviceCode
            Accessible.name: qsTrId("account.login.deviceCode")
        }
        TextField {
            Layout.fillWidth: true
            visible: AccountStore.verificationUrl.length > 0
            readOnly: true
            selectByMouse: true
            text: AccountStore.verificationUrl
            Accessible.name: qsTrId("account.login.link")
        }
        RowLayout {
            visible: AccountStore.verificationUrl.length > 0
            spacing: 12

            Button {
                text: qsTrId("account.login.copyLink")
                highlighted: true
                onClicked: Clipboard.copyText(AccountStore.verificationUrl)
            }
            Button {
                text: qsTrId("account.login.openBrowser")
                onClicked: Qt.openUrlExternally(AccountStore.verificationUrl)
            }
        }
        Label {
            Layout.fillWidth: true
            visible: AccountStore.errorCode.length > 0
            text: AccountStore.errorCode.length > 0 ? qsTrId(AccountStore.errorCode) : ""
            wrapMode: Text.WordWrap
            Accessible.role: Accessible.AlertMessage
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTrId("action.cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                if (AccountStore.busy)
                    AccountStore.cancel();
                else
                    dialog.reject();
            }
        }
        Button {
            text: dialog.microsoft ? qsTrId("account.login.start") : qsTrId("account.action.add")
            enabled: !AccountStore.busy && (dialog.microsoft || dialog.validOfflineName)
            highlighted: true
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: {
                if (dialog.microsoft)
                    AccountStore.loginMicrosoft(dialog.accountId);
                else
                    AccountStore.addOffline(offlineName.text);
            }
        }
    }
}