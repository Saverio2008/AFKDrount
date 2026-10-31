import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

Pane {
    id: root

    palette.window: Theme.accounts

    property var accounts: []
    property string selectedId: ""
    readonly property bool hasError: AccountStore.errorCode.length > 0 && AccountStore.errorCode !== "account.error.cancelled"

    signal accountSelected(string accountId)
    signal loginRequested(string accountId)
    signal addRequested

    signal deleteRequested(string entryId)

    function showMenu(entryId: string, anchor: Item, popupX: real, popupY: real) {
        entryMenu.record = root.accounts.find(entry => entry.id === entryId) || null;
        entryMenu.popup(anchor, popupX, popupY, null);
    }

    AccountContextMenu {
        id: entryMenu

        onDeleteRequested: entryId => root.deleteRequested(entryId)
        onLoginRequested: accountId => root.loginRequested(accountId)
        onAddRequested: root.addRequested()
    }

    padding: 12

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Label {
                Layout.fillWidth: true
                text: qsTrId("account.list.title")
                font.weight: Font.Medium
                elide: Text.ElideRight
            }
            ToolButton {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                padding: 4
                horizontalPadding: 4
                text: "↻"
                visible: root.hasError
                enabled: !AccountStore.busy
                Accessible.name: qsTrId("account.action.retry")
                ToolTip.visible: hovered
                ToolTip.text: qsTrId("account.action.retry")
                onClicked: AccountStore.reload()
            }
            ToolButton {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                padding: 4
                horizontalPadding: 4
                text: "＋"
                enabled: !AccountStore.busy
                Accessible.name: qsTrId("account.action.add")
                ToolTip.visible: hovered
                ToolTip.text: qsTrId("account.action.add")
                onClicked: root.addRequested()
            }
            ToolButton {
                id: moreButton

                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                padding: 4
                horizontalPadding: 4
                text: "⋯"
                Accessible.name: qsTrId("action.more")
                ToolTip.visible: hovered
                ToolTip.text: qsTrId("action.more")
                enabled: root.selectedId.length > 0
                onClicked: root.showMenu(root.selectedId, moreButton, 0, moreButton.height)
            }
        }

        Label {
            id: errorLabel

            Layout.fillWidth: true
            visible: root.hasError
            text: root.hasError ? qsTrId(AccountStore.errorCode) : ""
            maximumLineCount: 1
            elide: Text.ElideRight
            Accessible.role: Accessible.AlertMessage
            Accessible.name: text
            ToolTip.visible: errorHover.hovered
            ToolTip.text: text

            HoverHandler {
                id: errorHover
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            EmptyState {
                anchors.centerIn: parent
                width: parent.width
                visible: root.accounts.length === 0
                title: qsTrId("account.empty.title")
                description: qsTrId("account.empty.description")
            }

            SelectionList {
                anchors.fill: parent
                entries: root.accounts
                selectedId: root.selectedId
                showServer: false
                reorderEnabled: !AccountStore.busy
                moveEntry: (entryId, targetId, after) => {
                    AccountStore.moveAccount(entryId, targetId, after);
                    return true;
                }
                onEntryActivated: entryId => root.accountSelected(entryId)
                onContextRequested: (entryId, anchor, position) => root.showMenu(entryId, anchor, position.x, position.y)
                onEmptyContextRequested: (anchor, position) => root.showMenu("", anchor, position.x, position.y)
            }
        }
    }
}