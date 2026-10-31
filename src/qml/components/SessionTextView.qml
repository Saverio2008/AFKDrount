import QtQuick
import QtQuick.Controls

ScrollView {
    id: root
    property string pendingText: ""
    property bool richText: false
    property bool followEnd: false
    property string viewKey: ""
    readonly property alias hasSelection: area.selectedText
    readonly property var flickable: contentItem
    property string displayedText: ""
    property string displayedViewKey: ""

    //noinspection JSUnusedGlobalSymbols
    function copySelection() { area.copy(); }
    function liveTextDuringDrag() {
        if (!selectionPointer.active || !displayedText.length || pendingText.startsWith(displayedText))
            return root.pendingText;

        // Keep the drag anchor in place when the history drops old lines.
        const separator = richText ? "<br>" : "\n";
        const boundary = pendingText.indexOf(separator);
        const firstLine = boundary < 0 ? pendingText : pendingText.slice(0, boundary);
        for (let start = displayedText.indexOf(separator); start >= 0;
                start = displayedText.indexOf(separator, start + separator.length)) {
            const offset = start + separator.length;
            if (offset < displayedText.length && displayedText.startsWith(firstLine, offset)
                    && pendingText.startsWith(displayedText.slice(offset)))
                return displayedText + pendingText.slice(displayedText.length - offset);
        }
        return String(root.pendingText);
    }

    function retainedOffset(previous, current, start, end) {
        if (current.startsWith(previous.slice(0, end)))
            return 0;

        // Bounded history can drop old lines while new messages are appended.
        const head = current.slice(0, Math.min(64, Math.max(1, previous.length - end)));
        if (head.length > 0) {
            for (let offset = previous.indexOf(head); offset >= 0 && offset <= end;
                    offset = previous.indexOf(head, offset + 1)) {
                if (current.startsWith(previous.slice(offset)))
                    return -offset;
            }
        }
        const tail = previous.slice(start);
        return tail.length && current.endsWith(tail) ? current.length - previous.length : NaN;
    }

    function refresh(): void {
        const viewChanged = displayedViewKey !== viewKey;
        if (viewChanged) {
            displayedViewKey = viewKey;
            area.deselect();
        }
        const nextText = viewChanged ? pendingText : liveTextDuringDrag();
        if (displayedText === nextText)
            return;
        const atEnd = flickable.atYEnd || area.length === 0;
        const oldY = flickable.contentY;
        const start = area.selectionStart;
        const end = area.selectionEnd;
        const cursor = area.cursorPosition;
        const keepSelection = !viewChanged && (start !== end || selectionPointer.active);
        let offset = 0;
        if (!viewChanged && displayedText.length && nextText.startsWith(root.displayedText)) {
            area.insert(area.length, nextText.slice(displayedText.length));
        } else {
            const previous = keepSelection ? area.getText(0, area.length) : "";
            area.text = root.pendingText;
            if (keepSelection)
                offset = retainedOffset(previous, area.getText(0, area.length), start, end);
        }
        if (keepSelection && !isNaN(offset)) {
            const first = Math.max(0, start + offset);
            const last = Math.max(0, end + offset);
            const position = cursor === start ? first : last;
            if (area.selectionStart !== first || area.selectionEnd !== last || area.cursorPosition !== position)
                area.select(cursor === start ? last : first, position);
        }
        displayedText = nextText;
        Qt.callLater(() => {
            if (root.followEnd && (viewChanged || (atEnd && !keepSelection)))
                root.flickable.contentY = Math.max(0, root.flickable.contentHeight - root.flickable.height);
            else
                root.flickable.contentY = Math.min(oldY, Math.max(0, root.flickable.contentHeight - root.flickable.height));
        });
    }

    Component.onCompleted: {
        flickable.boundsBehavior = Flickable.StopAtBounds;
        Qt.callLater(root.refresh);
    }
    onPendingTextChanged: Qt.callLater(root.refresh)
    onViewKeyChanged: Qt.callLater(root.refresh)
    contentWidth: availableWidth
    rightPadding: bar.width + 4
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical: ScrollBar {
        id: bar
        parent: root
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }

    TextArea {
        id: area
        readOnly: true
        selectByMouse: true
        selectByKeyboard: true
        persistentSelection: true
        textFormat: root.richText ? TextEdit.RichText : TextEdit.PlainText
        wrapMode: TextEdit.WrapAnywhere
        PointHandler {
            id: selectionPointer
            acceptedButtons: Qt.LeftButton
            target: null
            onActiveChanged: {
                if (!active)
                    Qt.callLater(root.refresh);
            }
        }
        ContextMenu.menu: Menu {
            MenuItem {
                text: qsTrId("session.events.copySelection")
                enabled: area.selectedText.length > 0
                onTriggered: area.copy()
            }
            MenuItem {
                text: qsTrId("session.events.selectAll")
                enabled: area.length > 0
                onTriggered: area.selectAll()
            }
        }
    }
}
