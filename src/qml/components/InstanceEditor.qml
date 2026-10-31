import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

Pane {
    id: root

    palette.window: Theme.surface

    property var record: null
    property string displayedInstanceId: ""
    property var draftConfiguration: InstanceStore.defaultConfiguration()
    property int removedItems: 0
    readonly property var advancedSettings: advancedLoader.status === Loader.Ready ? advancedLoader.item : null
    onDraftConfigurationChanged: {
        if (advancedLoader.status === Loader.Ready && root.advancedSettings)
            root.advancedSettings.configuration = root.draftConfiguration;
    }
    readonly property string configurationErrorId: InstanceStore.configurationError(draftConfiguration)
    readonly property bool dirty: record !== null && (nameField.text !== record.name || serverField.text !== record.server || portField.text !== String(record.port) || JSON.stringify(draftConfiguration) !== JSON.stringify(record.configuration))
    readonly property bool valid: nameField.text.trim().length > 0 && serverField.text.trim().length > 0 && !(/[\s/\\]/.test(serverField.text)) && portField.acceptableInput && configurationErrorId.length === 0
    readonly property string draftName: nameField.text.trim()
    readonly property string draftServer: serverField.text.trim()
    readonly property int draftPort: Number(portField.text)

    signal saveRequested
    signal createRequested

    function focusName() {
        nameField.forceActiveFocus();
        nameField.selectAll();
    }

    function resetDraft() {
        nameField.text = record ? record.name : "";
        serverField.text = record ? record.server : "";
        portField.text = record ? String(record.port) : "25565";
        root.draftConfiguration = record ? JSON.parse(JSON.stringify(record.configuration)) : InstanceStore.defaultConfiguration();
        root.removedItems = 0;
    }

    function selectVersion(version: string) {
        const copy = JSON.parse(JSON.stringify(root.draftConfiguration));
        const filtered = GameCatalog.filterRules(version, copy.rules);
        copy.gameVersion = version;
        copy.rules = filtered.rules;
        root.removedItems = filtered.removedCount;
        root.draftConfiguration = copy;
    }

    onRecordChanged: {
        const nextId = root.record ? root.record.id : "";
        if (nextId !== root.displayedInstanceId) {
            advancedToggle.checked = false;
            root.displayedInstanceId = nextId;
        }
        root.resetDraft();
    }
    padding: 12

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: qsTrId("instance.editor.title")
                font.weight: Font.Medium
                elide: Text.ElideRight
            }

            Label {
                visible: root.record !== null && root.dirty
                text: qsTrId("instance.editor.unsaved")
                color: Theme.muted
            }

            ToolButton {
                visible: root.record !== null
                text: qsTrId("action.undo")
                padding: 4
                horizontalPadding: 8
                enabled: root.dirty
                onClicked: root.resetDraft()
            }

            Button {
                visible: root.record !== null
                text: qsTrId("action.save")
                padding: 4
                horizontalPadding: 12
                palette.button: Theme.mint
                palette.buttonText: Theme.mintText
                enabled: root.dirty && root.valid
                onClicked: root.saveRequested()
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.record === null

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width, 400)
                spacing: 20

                EmptyState {
                    Layout.fillWidth: true
                    title: qsTrId("instance.empty.title")
                    description: qsTrId("instance.empty.description")
                }

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTrId("instance.create.title")
                    onClicked: root.createRequested()
                }
            }
        }

        ScrollView {
            id: scroll
            readonly property var scrollableItem: contentItem
            Component.onCompleted: scrollableItem.boundsBehavior = Flickable.StopAtBounds

            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.record !== null
            contentWidth: availableWidth
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: scroll.availableWidth
                spacing: 8

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 8

                    Label {
                        text: qsTrId("instance.name")
                    }

                    CompactTextField {
                        id: nameField

                        Layout.fillWidth: true
                        maximumLength: 80
                        selectByMouse: true
                        placeholderText: qsTrId("instance.name.placeholder")
                        Accessible.name: qsTrId("instance.name")
                    }

                    Label {
                        text: qsTrId("instance.server")
                    }

                    CompactTextField {
                        id: serverField

                        Layout.fillWidth: true
                        selectByMouse: true
                        placeholderText: qsTrId("instance.server.placeholder")
                        Accessible.name: qsTrId("instance.server")
                    }

                    Label {
                        text: qsTrId("instance.port")
                    }

                    CompactTextField {
                        id: portField

                        Layout.preferredWidth: 160
                        selectByMouse: true
                        text: "25565"
                        placeholderText: qsTrId("input.example.number").arg(25565)
                        validator: IntValidator {
                            bottom: 1
                            top: 65535
                            locale: "C"
                        }
                        Accessible.name: qsTrId("instance.port.accessible")
                    }
                    Label {
                        text: qsTrId("config.version")
                    }
                    VersionSelector {
                        Layout.fillWidth: true
                        version: root.draftConfiguration.gameVersion
                        onSelected: value => root.selectVersion(value)
                    }
                }
                Label {
                    Layout.fillWidth: true
                    visible: root.removedItems > 0
                    text: qsTrId("catalog.item.removed").arg(root.removedItems)
                    color: Theme.muted
                    wrapMode: Text.WordWrap
                }
                DisclosureRow {
                    id: advancedToggle
                    Layout.fillWidth: true
                    text: qsTrId("config.advanced")
                }
                Loader {
                    id: advancedLoader
                    Layout.fillWidth: true
                    active: advancedToggle.checked
                    sourceComponent: AdvancedInstanceSettings {}
                    onLoaded: {
                        if (status === Loader.Ready && root.advancedSettings)
                            root.advancedSettings.configuration = root.draftConfiguration;
                    }
                }
                Connections {
                    target: root.advancedSettings
                    function onEdited(value) {
                        root.draftConfiguration = value;
                    }
                }
                Label {
                    Layout.fillWidth: true
                    visible: root.configurationErrorId.length > 0
                    text: root.configurationErrorId.length > 0 ? qsTrId(root.configurationErrorId) : ""
                    color: Theme.error
                    wrapMode: Text.WordWrap
                    Accessible.role: Accessible.AlertMessage
                }
            }
        }
    }
}
