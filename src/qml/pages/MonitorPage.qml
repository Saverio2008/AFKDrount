import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

Page {
    id: page
    property string sessionId: ""
    signal backRequested
    property bool logOpen: false
    property bool editing: false
    property var preferences: ({})
    property var order: [0, 1, 2]
    property int pendingRequest: 0
    property string pendingMode: ""
    property string errorCode: ""
    readonly property var record: {
        const revision = SessionStore.monitorRevision;
        return revision >= 0 && sessionId.length ? SessionStore.monitorRecord(sessionId) : ({});
    }
    readonly property var player: record.player || ({})
    readonly property var serverDisplay: record.serverDisplay || ({})
    readonly property var statistics: record.statistics || ({})
    readonly property bool sessionEnded: Boolean(record.id) && record.active === false
    readonly property var processUsage: record.resources || ({})
    readonly property var instance: record.instance || ({})
    readonly property var connection: record.connection && record.connection.server ? record.connection : instance
    readonly property var configuration: instance.configuration || ({})
    readonly property string version: record.version || configuration.gameVersion || ""
    readonly property bool commandSupported: GameCatalog.supportsCommands(version)
    readonly property string sessionRows: JSON.stringify(SessionStore.sessions.map(session => ({
        id: session.id,
        label: [session.accountName, session.instanceName, qsTrId("session.state." + session.state), session.startedAt].filter(Boolean).join(" · ")
    })))
    readonly property var choices: JSON.parse(sessionRows)
    readonly property var chat: record.chat || []
    readonly property string chatRevision: sessionId + "|" + chat.length + "|" +
        (chat.length ? String(chat[chat.length - 1].serial || chat[chat.length - 1].time) : "")
    property string chatMarkup: ""
    readonly property var paneNames: ["monitor.serverEvents",
        sessionEnded ? "monitor.production" : "monitor.statistics", "monitor.chat"]
    readonly property string serverText: ["title", "subtitle", "actionBar"].filter(key => serverDisplay[key])
        .map(key => qsTrId("session.event." + key) + " · " + serverDisplay[key]).join("\n") || "—"
    padding: 12

    function restoreLayout() {
        if (vertical.height <= 0 || horizontal.width <= 0)
            return;
        topSlot.SplitView.preferredHeight = Math.max(68, vertical.height * (preferences.topRatio || 0.24));
        leftSlot.SplitView.preferredWidth = horizontal.width * (preferences.leftRatio || 0.39);
    }

    function savePreferences(next) {
        if (!SessionStore.setMonitorPreferences(sessionId, next)) {
            errorCode = "session.error.save";
            return false;
        }
        errorCode = "";
        preferences = next;
        return true;
    }

    function saveSize() {
        if (editing && !savePreferences(Object.assign({}, preferences, {
                topRatio: topSlot.height / vertical.height,
                leftRatio: leftSlot.width / horizontal.width
            })))
            restoreLayout();
    }

    function swapPane(source: int, scenePoint: point) {
        const slots = [topSlot, leftSlot, rightSlot];
        for (let index = 0; index < slots.length; ++index) {
            const slot = slots[index];
            const point = slot.mapFromItem(null, scenePoint.x, scenePoint.y);
            if (index === source || !slot.contains(point))
                continue;
            const next = order.slice();
            [next[index], next[source]] = [next[source], next[index]];
            if (savePreferences(Object.assign({}, preferences, {order: next})))
                order = next;
            return;
        }
    }

    function changeSession() {
        pendingRequest = 0;
        pendingMode = "";
        errorCode = "";
        editing = false;
        chatInput.clear();
        commandInput.clear();
        const settings = SessionStore.monitorRecord(sessionId).monitor || {};
        preferences = settings;
        order = settings.order && settings.order.slice().sort().join(",") === "0,1,2"
            ? settings.order : [0, 1, 2];
        Qt.callLater(restoreLayout);
    }

    function submit(mode: string, text: string) {
        if (pendingRequest)
            return;
        pendingRequest = SessionStore.submitInput(sessionId, mode, text);
        pendingMode = pendingRequest ? mode : "";
        errorCode = pendingRequest ? "" : "monitor.inputFailed";
    }

    function shown(value) {
        return value === undefined || value === null || value === "" ? "—" : String(value);
    }

    function decimal(value, digits: int): string {
        return typeof value === "number" ? value.toFixed(digits) : "—";
    }

    function escapeText(value: string): string {
        return value.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/\r?\n/g, "<br>");
    }

    function formattedChat(entry) {
        if (!entry.motd)
            return escapeText(entry.text || "");
        const colors = ["#000000", "#0000aa", "#00aa00", "#00aaaa", "#aa0000", "#aa00aa", "#ffaa00", "#aaaaaa",
                        "#555555", "#5555ff", "#55ff55", "#55ffff", "#ff5555", "#ff55ff", "#ffff55", "#ffffff"];
        const marker = /§(#[0-9a-f]{6}|[0-9a-fklmnor])/ig;
        let styles = {};
        let result = "";
        let start = 0;
        let token;
        function span(text) {
            const style = Object.keys(styles).map(key => key + ":" + styles[key]).join(";");
            return text.length && style.length ? '<span style="' + style + '">' + escapeText(text) + "</span>" : escapeText(text);
        }
        while ((token = marker.exec(entry.motd)) !== null) {
            result += span(entry.motd.slice(start, token.index));
            start = marker.lastIndex;
            const code = token[1].toLowerCase();
            if (code === "r")
                styles = {};
            else if (code.startsWith("#"))
                styles.color = code;
            else if (/^[0-9a-f]$/.test(code))
                styles.color = colors[parseInt(code, 16)];
            else if (code === "l")
                styles["font-weight"] = "bold";
            else if (code === "o")
                styles["font-style"] = "italic";
            else if (code === "n" || code === "m") {
                const decoration = code === "n" ? "underline" : "line-through";
                if (!(styles["text-decoration"] || "").includes(decoration))
                    styles["text-decoration"] = ((styles["text-decoration"] || "") + " " + decoration).trim();
            }
        }
        return result + span(entry.motd.slice(start));
    }

    Component.onCompleted: { changeSession(); }
    onSessionIdChanged: if (chatInput && commandInput) changeSession()
    onChatRevisionChanged: chatMarkup = chat.map(entry => formattedChat(entry)).join("<br>")
    Connections {
        target: SessionStore
        function onInputFinished(id, request, code) {
            if (id !== page.sessionId || request !== page.pendingRequest)
                return;
            if (!code.length) {
                if (page.pendingMode === "chat")
                    chatInput.clear();
                else
                    commandInput.clear();
            }
            page.errorCode = code;
            page.pendingRequest = 0;
        }
    }

    ApplySessionConfigurationDialog { id: applyDialog }
    Menu {
        id: privacy
        readonly property var scrollableMenu: contentItem
        Component.onCompleted: scrollableMenu.boundsBehavior = Flickable.StopAtBounds
        MenuItem {
            text: qsTrId("monitor.hidePosition")
            checkable: true
            checked: page.preferences.hidePosition !== false
            onTriggered: {
                if (!page.savePreferences(Object.assign({}, page.preferences, {hidePosition: checked})))
                    checked = page.preferences.hidePosition !== false;
            }
        }
        MenuItem {
            text: qsTrId("monitor.hideAccount")
            checkable: true
            checked: Boolean(page.preferences.hideAccount)
            onTriggered: {
                if (!page.savePreferences(Object.assign({}, page.preferences, {hideAccount: checked})))
                    checked = Boolean(page.preferences.hideAccount);
            }
        }
        MenuItem {
            text: qsTrId("monitor.hideServer")
            checkable: true
            checked: Boolean(page.preferences.hideServer)
            onTriggered: {
                if (!page.savePreferences(Object.assign({}, page.preferences, {hideServer: checked})))
                    checked = Boolean(page.preferences.hideServer);
            }
        }
        MenuItem {
            text: qsTrId("monitor.hideCommand")
            checkable: true
            checked: Boolean(page.preferences.hideCommand)
            onTriggered: {
                if (!page.savePreferences(Object.assign({}, page.preferences, {hideCommand: checked})))
                    checked = Qt.binding(() => Boolean(page.preferences.hideCommand));
            }
        }
    }

    component SplitGrip: Rectangle {
        id: grip
        required property bool interactive

        implicitWidth: 8
        implicitHeight: 8
        color: interactive ? Theme.outline : "transparent"
        // Keep the handle alive when SplitView updates the panes' visibility.
        containmentMask: Item {
            width: grip.interactive ? grip.width : 0
            height: grip.height
        }
    }

    component Panel: Pane {
        id: panel
        required property int slotIndex
        required property var monitor
        readonly property int paneIndex: monitor.order[slotIndex]
        padding: 8
        background: Rectangle {
            color: Theme.surface
            radius: Theme.rowRadius
            border.color: Theme.outline
        }
        ColumnLayout {
            anchors.fill: parent
            spacing: 4
            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: qsTrId(panel.monitor.paneNames[panel.paneIndex])
                    font.weight: Font.Medium
                }
                Label {
                    visible: panel.monitor.editing
                    text: "⠿"
                    Accessible.name: qsTrId("monitor.drag")
                    DragHandler {
                        id: drag
                        enabled: panel.monitor.editing
                        target: null
                        onActiveChanged: {
                            if (!active && enabled)
                                panel.monitor.swapPane(panel.slotIndex, centroid.scenePosition);
                        }
                    }
                }
            }
            Loader {
                id: body
                readonly property var view: item
                Layout.fillWidth: true
                Layout.fillHeight: true
                sourceComponent: panel.paneIndex === 1 ? statsPane : textPane
                onLoaded: {
                    if (panel.paneIndex === 1) {
                        view.statistics = Qt.binding(() => panel.monitor.statistics);
                        view.sessionEnded = Qt.binding(() => panel.monitor.sessionEnded);
                        view.gameVersion = Qt.binding(() => panel.monitor.version);
                    } else {
                        view.richText = Qt.binding(() => panel.paneIndex === 2);
                        view.followEnd = Qt.binding(() => panel.paneIndex === 2);
                        view.viewKey = Qt.binding(() => panel.monitor.sessionId + ":" + panel.paneIndex);
                        view.pendingText = Qt.binding(() => panel.paneIndex === 2 ? panel.monitor.chatMarkup : panel.monitor.serverText);
                    }
                }
            }
        }
        Component {
            id: statsPane
            SessionStatistics {}
        }
        Component {
            id: textPane
            SessionTextView {}
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6
        RowLayout {
            Layout.fillWidth: true
            ToolButton {
                text: "‹ " + qsTrId(page.logOpen ? "action.monitor" : "monitor.back")
                onClicked: {
                    if (page.logOpen)
                        page.logOpen = false;
                    else
                        page.backRequested();
                }
            }
            ComboBox {
                id: selector
                readonly property var scrollablePopup: popup.contentItem
                Component.onCompleted: scrollablePopup.boundsBehavior = Flickable.StopAtBounds
                Layout.fillWidth: true
                model: page.choices
                textRole: "label"
                currentIndex: page.choices.findIndex(entry => entry.id === page.sessionId)
                Accessible.name: qsTrId("monitor.session")
                onActivated: index => page.sessionId = page.choices[index].id
            }
            ToolButton {
                text: qsTrId("session.events.title")
                visible: !page.logOpen
                onClicked: page.logOpen = true
            }
            ToolButton {
                text: qsTrId("session.apply.action")
                visible: Boolean(page.record.active) && !page.logOpen
                enabled: !page.record.applying && page.record.state !== "stopping"
                onClicked: { applyDialog.sessionId = page.sessionId; applyDialog.open(); }
            }
            Button {
                text: qsTrId("action.stop")
                visible: Boolean(page.record.active) && !page.logOpen
                enabled: page.record.state !== "stopping"
                onClicked: SessionStore.stopSession(page.sessionId)
            }
        }
        Label {
            visible: page.errorCode.length > 0
            text: visible ? qsTrId(page.errorCode) : ""
            Accessible.role: Accessible.AlertMessage
        }
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: page.logOpen ? 1 : 0
            ColumnLayout {
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        textFormat: Text.PlainText
                        text: qsTrId("monitor.overview")
                            .arg(page.connection.server ? page.connection.server + ":" + page.connection.port : "—")
                            .arg(page.shown(page.version))
                            .arg(Math.floor((page.statistics.onlineMilliseconds || 0) / 1000))
                            .arg(page.record.retries || 0)
                            .arg(page.decimal(page.processUsage.cpu, 1))
                            .arg(page.decimal(typeof page.processUsage.memory === "number" ? page.processUsage.memory / 1048576 : null, 1))
                    }
                    ToolButton {
                        text: qsTrId("monitor.privacy")
                        enabled: Boolean(page.record.id)
                        onClicked: privacy.popup()
                    }
                    Switch {
                        text: qsTrId("monitor.editLayout")
                        checked: page.editing
                        onToggled: page.editing = checked
                    }
                    ToolButton {
                        text: qsTrId("monitor.resetLayout")
                        enabled: Boolean(page.record.id)
                        onClicked: {
                            if (page.savePreferences(Object.assign({}, page.preferences, {order: [0, 1, 2], topRatio: 0.24, leftRatio: 0.39}))) {
                                page.order = [0, 1, 2];
                                page.restoreLayout();
                            }
                        }
                    }
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    textFormat: Text.PlainText
                    text: [
                        qsTrId("monitor.health").arg(page.decimal(page.player.health, 1)).arg(page.decimal(page.player.maxHealth, 1)),
                        qsTrId("monitor.food").arg(page.shown(page.player.food)).arg(page.decimal(page.player.saturation, 1)),
                        qsTrId("monitor.experience").arg(page.shown(page.player.level))
                            .arg(page.decimal(typeof page.player.experience === "number" ? page.player.experience * 100 : null, 0)),
                        qsTrId("monitor.ping").arg(page.shown(page.player.ping)),
                        page.shown(page.player.dimension), page.shown(page.player.gameMode),
                        page.preferences.hidePosition === false ? "XYZ " + (page.player.position || ["—"]).map(value => typeof value === "number" ? page.decimal(value, 1) : value).join(", ") : "",
                        qsTrId("monitor.sampledAt").arg(page.shown(page.player.sampledAt))
                    ].filter(Boolean).join(" · ")
                }
                SplitView {
                    id: vertical
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: Qt.Vertical
                    onHeightChanged: if (!resizing) Qt.callLater(page.restoreLayout)
                    onResizingChanged: if (!resizing) page.saveSize()
                    handle: SplitGrip { interactive: page.editing }
                    Panel {
                        id: topSlot
                        monitor: page
                        slotIndex: 0
                        SplitView.minimumHeight: 68
                    }
                    SplitView {
                        id: horizontal
                        SplitView.fillHeight: true
                        SplitView.minimumHeight: 100
                        onWidthChanged: if (!resizing) Qt.callLater(page.restoreLayout)
                        onResizingChanged: if (!resizing) page.saveSize()
                        handle: SplitGrip { interactive: page.editing }
                        Panel {
                            id: leftSlot
                            monitor: page
                            slotIndex: 1
                            SplitView.minimumWidth: 180
                        }
                        Panel {
                            id: rightSlot
                            monitor: page
                            slotIndex: 2
                            SplitView.minimumWidth: 180
                            SplitView.fillWidth: true
                        }
                    }
                }
                RowLayout {
                    Label { text: qsTrId("monitor.chat") }
                    CompactTextField {
                        id: chatInput
                        Layout.fillWidth: true
                        maximumLength: 256
                        enabled: Boolean(page.record.inputReady)
                        readOnly: page.pendingRequest > 0
                        placeholderText: qsTrId("monitor.chat")
                        Accessible.name: placeholderText
                        onAccepted: if (chatSend.enabled) page.submit("chat", text)
                    }
                    Button {
                        id: chatSend
                        text: qsTrId("monitor.sendChat")
                        enabled: chatInput.enabled && !page.pendingRequest && chatInput.text.trim().length > 0
                            && !chatInput.text.trim().startsWith("/")
                        onClicked: page.submit("chat", chatInput.text)
                    }
                }
                RowLayout {
                    Label { text: qsTrId("monitor.command") }
                    CompactTextField {
                        id: commandInput
                        Layout.fillWidth: true
                        maximumLength: 256
                        echoMode: page.preferences.hideCommand ? TextInput.Password : TextInput.Normal
                        enabled: Boolean(page.record.commandReady)
                        readOnly: page.pendingRequest > 0
                        placeholderText: page.version.length && !page.commandSupported ? qsTrId("task.error.commandVersion") : qsTrId("monitor.command")
                        Accessible.name: qsTrId("monitor.command")
                        onAccepted: if (commandSend.enabled) page.submit("command", text)
                    }
                    Button {
                        id: commandSend
                        text: qsTrId("monitor.sendCommand")
                        enabled: commandInput.enabled && !page.pendingRequest && commandInput.text.trim().length > 0
                        onClicked: page.submit("command", commandInput.text)
                    }
                }
            }
            SessionLogView {
                sessionId: page.sessionId
                events: page.logOpen ? page.record.events || [] : []
            }
        }
    }
}
