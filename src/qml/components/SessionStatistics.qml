import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

ScrollView {
    id: root
    property var statistics: ({})
    property bool sessionEnded: false
    property string gameVersion: ""
    readonly property string gameLocale: LanguageManager.minecraftLocale
    readonly property int catalogRevision: GameCatalog.revision
    readonly property var rules: (statistics.rules || []).concat(statistics.archivedSegments || [])
    readonly property string ruleKeys: JSON.stringify(rules.map(rule => [rule.ruleId, rule.segment]))
    readonly property var scrollableItem: contentItem
    onGameVersionChanged: GameCatalog.ensureLanguage(gameVersion, gameLocale)
    onGameLocaleChanged: GameCatalog.ensureLanguage(gameVersion, gameLocale)
    Component.onCompleted: {
        scrollableItem.boundsBehavior = Flickable.StopAtBounds;
        GameCatalog.ensureLanguage(gameVersion, gameLocale);
    }
    contentWidth: availableWidth
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    ColumnLayout {
        property var statisticsView: root
        width: root.availableWidth
        Label {
            Layout.fillWidth: true
            visible: root.sessionEnded && !root.rules.some(rule => Object.keys(rule.items || {}).length > 0)
            text: qsTrId("monitor.noProduction")
            color: Theme.muted
            wrapMode: Text.WordWrap
        }
        Repeater {
            model: JSON.parse(root.ruleKeys).length
            delegate: ColumnLayout {
                id: rule
                required property int index
                readonly property var owner: parent
                readonly property var statisticsView: owner.statisticsView
                readonly property var modelData: statisticsView.rules[index] || ({})
                readonly property bool archived: index >= (statisticsView.statistics.rules || []).length
                readonly property string itemKeys: JSON.stringify(Object.keys(modelData.items || {}))
                visible: !statisticsView.sessionEnded || itemKeys !== "[]"

                ToolButton {
                    Layout.fillWidth: true
                    checkable: true
                    checked: rule.statisticsView.sessionEnded || !rule.archived
                    text: (checked ? "▾ " : "▸ ") + qsTrId("monitor.rule").arg(rule.modelData.name).arg(rule.modelData.segment)
                    id: disclosure
                }
                Repeater {
                    model: disclosure.checked ? JSON.parse(rule.itemKeys) : []
                    delegate: RowLayout {
                        id: itemRow
                        required property string modelData
                        readonly property var owner: parent
                        readonly property bool custom: modelData.startsWith("custom:")
                        readonly property var units: owner.modelData.units && owner.modelData.units[modelData]
                        readonly property url iconUrl: custom ? "" : MinecraftImages.itemIcon(modelData)

                        Image {
                            id: itemIcon
                            //noinspection UnnecessaryLabelJS
                            source: itemRow.iconUrl
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            visible: !itemRow.custom && itemIcon.status === Image.Ready
                            smooth: false
                        }
                        Label {
                            Layout.fillWidth: true
                            text: {
                                const view = itemRow.owner.statisticsView;
                                if (itemRow.custom)
                                    return itemRow.modelData.slice(7);
                                return view.catalogRevision >= 0
                                    ? GameCatalog.itemLabel(view.gameVersion, itemRow.modelData, view.gameLocale)
                                    : itemRow.modelData;
                            }
                            textFormat: Text.PlainText
                            elide: Text.ElideRight
                        }
                        Label {
                            text: (itemRow.owner.modelData.items || {})[itemRow.modelData] || ""
                        }
                        Label {
                            visible: Boolean(itemRow.units)
                            text: itemRow.units ? qsTrId("monitor.units").arg(itemRow.units.boxes).arg(itemRow.units.stacks).arg(itemRow.units.items) : ""
                            color: Theme.muted
                        }
                    }
                }
            }
        }
    }
}
