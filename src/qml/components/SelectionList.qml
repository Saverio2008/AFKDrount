import QtQuick
import QtQuick.Controls

ListView {
    id: list
    boundsBehavior: Flickable.StopAtBounds

    required property var entries
    required property var moveEntry
    property string selectedId: ""
    property bool showServer: false
    property bool reorderEnabled: true

    property int draggedIndex: -1
    property int destinationIndex: -1
    property point pointerPosition
    property point grabPosition
    property real draggedHeight: 0
    property string pendingMoveId: ""
    readonly property bool reordering: draggedIndex >= 0
    readonly property Item dragLayer: Window.window ? Window.window.contentItem : null
    readonly property var rowSlots: contentItem.children
    readonly property bool pointerInside: pointerPosition.x >= 0 && pointerPosition.x < width
                                          && pointerPosition.y >= 0 && pointerPosition.y < height

    signal entryActivated(string entryId)
    signal contextRequested(string entryId, Item anchor, point position)
    signal emptyContextRequested(Item anchor, point position)

    function requestMove(entryId: string, targetId: string, after: bool) {
        list.pendingMoveId = entryId;
        if (!list.moveEntry(entryId, targetId, after))
            list.pendingMoveId = "";
    }

    function revealEntry(entryId: string) {
        const index = list.entries.findIndex(entry => entry.id === entryId);
        if (index >= 0) {
            list.positionViewAtIndex(index, ListView.Contain);
            list.currentIndex = index;
        }
    }

    function updateTarget() {
        if (!list.pointerInside) {
            list.destinationIndex = list.draggedIndex;
            return;
        }

        const dragCenter = list.pointerPosition.y - list.grabPosition.y + list.draggedHeight / 2;
        const y = list.contentY + Math.max(0, Math.min(list.height - 1, dragCenter));
        let next = list.count;
        let previous = -1;
        for (const slot of list.rowSlots) {
            if (typeof slot.previewOffset !== "number" || slot.index === list.draggedIndex)
                continue;
            if (y < slot.y + slot.previewOffset + slot.height / 2)
                next = Math.min(next, slot.index);
            else
                previous = Math.max(previous, slot.index);
        }
        if (next < list.count)
            list.destinationIndex = next - (list.draggedIndex < next ? 1 : 0);
        else if (previous >= 0)
            list.destinationIndex = previous + (list.draggedIndex > previous ? 1 : 0);
    }

    function finishDrag(commit: bool) {
        if (!list.reordering)
            return;
        const shouldMove = commit && list.destinationIndex !== list.draggedIndex && list.reorderEnabled;
        const entryId = shouldMove ? list.entries[list.draggedIndex].id : "";
        const targetId = shouldMove ? list.entries[list.destinationIndex].id : "";
        const after = list.destinationIndex > list.draggedIndex;
        list.draggedIndex = -1;
        list.destinationIndex = -1;
        if (shouldMove)
            list.requestMove(entryId, targetId, after);
    }

    onEntriesChanged: {
        list.finishDrag(false);
        if (list.pendingMoveId) {
            Qt.callLater(list.revealEntry, list.pendingMoveId);
            list.pendingMoveId = "";
        }
    }
    onReorderEnabledChanged: {
        if (list.reorderEnabled)
            list.pendingMoveId = "";
        else
            list.finishDrag(false);
    }
    onContentYChanged: if (reordering) updateTarget()
    Keys.onEscapePressed: event => {
        event.accepted = list.reordering;
        list.finishDrag(false);
    }

    ContextMenu.onRequested: position => list.emptyContextRequested(list, position)

    clip: true
    acceptedButtons: Qt.NoButton
    interactive: !reordering
    model: entries
    spacing: 10
    ScrollBar.vertical: ScrollBar {}

    Timer {
        interval: 16
        repeat: true
        running: list.reordering && list.pointerInside
                 && (list.pointerPosition.y < 24 || list.pointerPosition.y > list.height - 24)
        onTriggered: {
            const direction = list.pointerPosition.y < 24 ? -1 : 1;
            const bottom = Math.max(list.originY, list.originY + list.contentHeight - list.height);
            list.contentY = Math.max(list.originY, Math.min(bottom, list.contentY + direction * 8));
        }
    }

    delegate: Item {
        id: entry

        required property int index
        required property var modelData
        readonly property var owner: ListView.view
        readonly property bool held: owner.draggedIndex === index
        readonly property real previewOffset: {
            if (!entry.owner.reordering)
                return 0;
            const extent = entry.owner.draggedHeight + entry.owner.spacing;
            if (entry.index > entry.owner.draggedIndex && entry.index <= entry.owner.destinationIndex)
                return -extent;
            if (entry.index < entry.owner.draggedIndex && entry.index >= entry.owner.destinationIndex)
                return extent;
            return 0;
        }

        width: entry.owner.width
        height: row.implicitHeight
        z: held ? 2 : 1

        ItemDelegate {
            id: row

            readonly property bool microsoft: !entry.owner.showServer && entry.modelData.type === "microsoft"
            readonly property int avatarRevision: MinecraftImages.revision
            readonly property point dragPosition: entry.held
                ? entry.owner.mapToItem(row.parent, entry.owner.pointerPosition.x - entry.owner.grabPosition.x,
                                 entry.owner.pointerPosition.y - entry.owner.grabPosition.y)
                : Qt.point(0, 0)

            function loadAvatar() {
                if (row.microsoft)
                    MinecraftImages.ensureAvatar(entry.modelData.uuid);
            }

            Component.onCompleted: {
                row.loadAvatar();
            }
            Connections {
                target: entry
                function onModelDataChanged() { row.loadAvatar(); }
            }
            Connections {
                target: AccountStore
                function onAccountSaved(accountId: string) {
                    if (row.microsoft && entry.modelData.id === accountId)
                        MinecraftImages.ensureAvatar(entry.modelData.uuid, true);
                }
            }

            parent: entry.held && entry.owner.dragLayer ? entry.owner.dragLayer : entry
            palette: entry.palette
            width: entry.width
            x: dragPosition.x
            y: dragPosition.y
            z: entry.held ? 1 : 0
            transform: Translate {
                y: entry.previewOffset
                Behavior on y {
                    enabled: entry.owner.reordering && !entry.held
                    NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
                }
            }
            down: pressed || (pointer.pressed && pointer.containsMouse && !entry.owner.reordering)
            icon.source: microsoft && avatarRevision >= 0 ? MinecraftImages.avatar(entry.modelData.uuid) : ""
            text: entry.owner.showServer
                  ? "%1\n%2 · %3".arg(entry.modelData.name).arg(entry.modelData.server).arg(entry.modelData.port)
                  : entry.modelData.name
            highlighted: entry.held || entry.modelData.id === entry.owner.selectedId
            onClicked: entry.owner.entryActivated(entry.modelData.id)
            Keys.onPressed: event => {
                if (!entry.owner.reorderEnabled || entry.owner.reordering || event.modifiers !== Qt.AltModifier)
                    return;
                const direction = event.key === Qt.Key_Up ? -1 : event.key === Qt.Key_Down ? 1 : 0;
                const target = entry.owner.entries[entry.index + direction];
                if (direction && target) {
                    event.accepted = true;
                    entry.owner.requestMove(entry.modelData.id, target.id, direction > 0);
                }
            }
        }

        MouseArea {
            id: pointer

            property point pressPosition
            property bool dragStarted: false

            anchors.fill: parent
            z: 1
            preventStealing: entry.held
            cursorShape: entry.held ? Qt.ClosedHandCursor : Qt.ArrowCursor

            onPressed: mouse => {
                pointer.pressPosition = Qt.point(mouse.x, mouse.y);
                pointer.dragStarted = false;
            }
            onClicked: {
                if (!pointer.dragStarted) {
                    row.forceActiveFocus(Qt.MouseFocusReason);
                    row.click();
                }
            }
            onPositionChanged: mouse => {
                if (pointer.pressed && !pointer.dragStarted && entry.owner.reorderEnabled && entry.owner.count > 1
                        && Math.hypot(mouse.x - pointer.pressPosition.x, mouse.y - pointer.pressPosition.y) >= pointer.drag.threshold) {
                    pointer.dragStarted = true;
                    entry.owner.forceActiveFocus(Qt.MouseFocusReason);
                    entry.owner.grabPosition = pointer.pressPosition;
                    entry.owner.draggedHeight = entry.height;
                    entry.owner.destinationIndex = entry.index;
                    entry.owner.draggedIndex = entry.index;
                    // ListView retains its current delegate during edge scrolling.
                    entry.owner.currentIndex = entry.index;
                }
                if (entry.held) {
                    entry.owner.pointerPosition = pointer.mapToItem(entry.owner, mouse.x, mouse.y);
                    entry.owner.updateTarget();
                }
            }
            onReleased: mouse => {
                if (entry.held) {
                    entry.owner.pointerPosition = pointer.mapToItem(entry.owner, mouse.x, mouse.y);
                    entry.owner.updateTarget();
                    entry.owner.finishDrag(true);
                }
            }
            onCanceled: if (entry.held) entry.owner.finishDrag(false)
        }

        ContextMenu.onRequested: position => entry.owner.contextRequested(entry.modelData.id, entry, position)
    }
}
