import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

Pane {
    id: root

    palette.window: Theme.sessions

    property var sessions: []
    property string accountName: ""
    property string instanceName: ""
    property bool hasUnsavedChanges: false
    property bool configurationReady: false

    signal runRequested
    signal stopRequested(string sessionId)
    signal monitorRequested(string sessionId)
    signal applyRequested(string sessionId)

    padding: 12

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: qsTrId("session.list.title")
                font.weight: Font.Medium
            }

            Label {
                text: qsTrId("session.count").arg(SessionStore.activeCount)
                color: Theme.muted
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 54

            Label {
                anchors.centerIn: parent
                width: parent.width
                visible: root.sessions.length === 0
                text: qsTrId("session.empty")
                horizontalAlignment: Text.AlignHCenter
                color: Theme.muted
            }

            SessionList {
                anchors.fill: parent
                visible: root.sessions.length > 0
                model: root.sessions
                onStopRequested: sessionId => root.stopRequested(sessionId)
                onMonitorRequested: sessionId => root.monitorRequested(sessionId)
                onApplyRequested: sessionId => root.applyRequested(sessionId)
            }
        }

        Label {
            Layout.fillWidth: true
            visible: SessionStore.errorCode.length > 0
            text: visible ? qsTrId(SessionStore.errorCode) : ""
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: root.accountName.length > 0 ? qsTrId("session.selectedAccount").arg(root.accountName) : qsTrId("session.noAccount")
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: root.instanceName.length > 0 ? qsTrId("session.selectedInstance").arg(root.instanceName) : qsTrId("session.noInstance")
                    elide: Text.ElideRight
                    color: Theme.muted
                }
            }

            Button {
                text: qsTrId("session.action.run")
                highlighted: true
                Layout.preferredWidth: 132
                enabled: root.accountName.length > 0 && root.instanceName.length > 0 && root.configurationReady && !AccountStore.busy
                ToolTip.visible: hovered && root.instanceName.length > 0 && (!root.configurationReady || root.hasUnsavedChanges)
                ToolTip.text: qsTrId(root.configurationReady ? "session.run.unsavedHint" : "config.error.version")
                onClicked: root.runRequested()
            }
        }
    }
}
