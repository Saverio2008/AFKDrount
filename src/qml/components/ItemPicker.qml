import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

ColumnLayout {
    id: root
    property string gameVersion: ""
    property var selectedItems: []
    property var matches: []
    property var selectedLabels: ({})
    property bool componentReady: false
    property bool languageBusy: false
    property string languageError: ""
    readonly property string locale: LanguageManager.minecraftLocale
    signal edited(var value)

    function refresh() {
        root.matches = GameCatalog.searchItems(root.gameVersion, search.text, root.locale);
        root.languageBusy = GameCatalog.languageBusy(root.gameVersion, root.locale);
        root.languageError = GameCatalog.languageError(root.gameVersion, root.locale);
        const labels = {};
        for (const name of root.selectedItems)
            labels[name] = GameCatalog.itemLabel(root.gameVersion, name, root.locale);
        root.selectedLabels = labels;
    }

    onGameVersionChanged: {
        if (root.componentReady) {
            GameCatalog.ensureLanguage(root.gameVersion, root.locale);
            root.refresh();
        }
    }
    onSelectedItemsChanged: {
        if (root.componentReady)
            root.refresh();
    }
    Component.onCompleted: {
        root.componentReady = true;
        GameCatalog.ensureLanguage(root.gameVersion, root.locale);
        root.refresh();
    }
    Connections {
        target: GameCatalog
        //noinspection JSUnusedGlobalSymbols
        function onChanged() {
            if (root.componentReady)
                root.refresh();
        }
    }
    onLocaleChanged: {
        if (root.componentReady) {
            GameCatalog.ensureLanguage(root.gameVersion, root.locale);
            root.refresh();
        }
    }
    spacing: 6

    Flow {
        id: selectedFlow
        Layout.fillWidth: true
        property string gameVersion: root.gameVersion
        property var labels: root.selectedLabels
        signal removeItem(string name)
        onRemoveItem: name => root.edited(root.selectedItems.filter(item => item !== name))
        spacing: 4
        Repeater {
            model: root.selectedItems
            delegate: ToolButton {
                id: chip
                required property string modelData
                readonly property var owner: parent
                text: (chip.owner.labels[chip.modelData] || chip.modelData) + " · " + chip.modelData + " ×"
                padding: 4
                horizontalPadding: 8
                Accessible.name: qsTrId("catalog.item.remove").arg(chip.modelData)
                onClicked: chip.owner.removeItem(chip.modelData)
            }
        }
    }
    CompactTextField {
        id: search
        Layout.fillWidth: true
        placeholderText: qsTrId("catalog.item.search")
        Accessible.name: qsTrId("catalog.item.search")
        onTextEdited: root.refresh()
    }
    RowLayout {
        Layout.fillWidth: true
        visible: root.languageBusy || root.languageError.length > 0
        Label {
            Layout.fillWidth: true
            text: root.languageBusy ? qsTrId("catalog.language.loading") : root.languageError.length > 0 ? qsTrId(root.languageError) : ""
            color: Theme.muted
            wrapMode: Text.WordWrap
        }
        ToolButton {
            visible: root.languageError.length > 0 && !root.languageBusy
            text: qsTrId("account.action.retry")
            onClicked: GameCatalog.ensureLanguage(root.gameVersion, root.locale)
        }
    }
    ListView {
        id: list
        boundsBehavior: Flickable.StopAtBounds
        Layout.fillWidth: true
        Layout.preferredHeight: 190
        property var selectedItems: root.selectedItems
        signal toggleItem(string name)
        onToggleItem: name => root.edited(root.selectedItems.includes(name) ? root.selectedItems.filter(item => item !== name) : root.selectedItems.concat([name]))
        model: root.matches
        clip: true
        ScrollBar.vertical: ScrollBar {}
        delegate: ItemDelegate {
            id: option
            required property var modelData
            readonly property var owner: ListView.view
            width: owner.width
            checkable: true
            checked: owner.selectedItems.includes(modelData.name)
            highlighted: checked
            icon.source: MinecraftImages.itemIcon(modelData.name)
            text: modelData.displayName + "\nminecraft:" + modelData.name
            onClicked: option.owner.toggleItem(option.modelData.name)
        }
        Label {
            anchors.centerIn: parent
            visible: root.matches.length === 0
            text: qsTrId("catalog.item.empty")
            color: Theme.muted
        }
    }
    Label {
        Layout.fillWidth: true
        visible: GameCatalog.legacy(root.gameVersion)
        text: qsTrId("catalog.item.legacy")
        color: Theme.muted
        wrapMode: Text.WordWrap
    }
}
