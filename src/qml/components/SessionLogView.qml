import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    property var events: []
    property string sessionId: ""
    readonly property var filters: ["all", "chat", "server", "status", "sending", "errors"]
    readonly property string logText: events.filter(event => filter.currentIndex === 0 ||
        root.category(event.kind) === filters[filter.currentIndex] ||
        (filters[filter.currentIndex] === "errors" && ["taskFailed", "manualFailed"].includes(event.kind))
    ).map(event => {
        const data = event.data || {};
        let detail = event.text || "";
        if (event.kind === "error" || event.kind === "applyRejected")
            detail = detail.length ? qsTrId(detail) : "";
        else if (event.kind === "taskFailed")
            detail += " · " + qsTrId(data.code);
        else if (event.kind === "taskStepSent")
            detail += " · " + String(data.step);
        else if (event.kind === "manualSent" || event.kind === "manualFailed")
            detail = qsTrId("monitor." + data.mode) + (data.code ? " · " + qsTrId(data.code) : "");
        return qsTrId("session.event.line").arg(event.time).arg(qsTrId("session.event." + event.kind)).arg(detail);
    }).join("\n")

    function category(kind: string): string {
        if (kind === "chat")
            return "chat";
        if (["title", "subtitle", "title_clear", "title_times", "actionBar"].includes(kind))
            return "server";
        if (kind.startsWith("task") || kind.startsWith("manual"))
            return "sending";
        if (["error", "failed", "timeout", "applyRejected", "connectionError", "kicked", "authentication", "configuration"].includes(kind))
            return "errors";
        return "status";
    }

    RowLayout {
        ComboBox {
            id: filter
            readonly property var scrollablePopup: popup.contentItem
            Component.onCompleted: scrollablePopup.boundsBehavior = Flickable.StopAtBounds
            model: root.filters.map(kind => qsTrId("monitor.filter." + kind))
            Accessible.name: qsTrId("monitor.logFilter")
        }
        Item { Layout.fillWidth: true }
        ToolButton {
            text: qsTrId("session.events.copySelection")
            enabled: logs.hasSelection.length > 0
            onClicked: logs.copySelection()
        }
        ToolButton {
            text: qsTrId("monitor.copyFiltered")
            enabled: root.logText.length > 0
            onClicked: Clipboard.copyText(root.logText)
        }
    }

    SessionTextView {
        id: logs
        Layout.fillWidth: true
        Layout.fillHeight: true
        pendingText: root.logText
        viewKey: root.sessionId + ":" + filter.currentIndex
    }
}