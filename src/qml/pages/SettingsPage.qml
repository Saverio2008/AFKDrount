import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AppAppearance

Page {
    id: page

    signal backRequested

    padding: Theme.gutter

    header: ToolBar {
        padding: 16

        RowLayout {
            anchors.fill: parent
            spacing: 12

            ToolButton {
                text: "←"
                Accessible.name: qsTrId("action.back")
                ToolTip.visible: hovered
                ToolTip.text: qsTrId("action.back")
                onClicked: page.backRequested()
            }
            Label {
                Layout.fillWidth: true
                text: qsTrId("settings.title")
                font.pointSize: 18
                font.weight: Font.Medium
            }
        }
    }

    ScrollView {
        id: scroll
        implicitWidth: 0
        readonly property var scrollableItem: contentItem
        Component.onCompleted: scrollableItem.boundsBehavior = Flickable.StopAtBounds

        anchors.fill: parent
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: scroll.availableWidth
            spacing: Theme.gutter

            Pane {
                Layout.fillWidth: true
                padding: 28
                palette.window: Theme.surface
                ColumnLayout {
                    width: parent.width
                    spacing: 8
                    Label {
                        text: qsTrId("settings.sessions")
                        font.weight: Font.Medium
                    }
                    CheckBox {
                        text: qsTrId("session.close.confirm")
                        checked: SessionStore.confirmClose
                        onClicked: SessionStore.confirmClose = checked
                    }
                    CheckBox {
                        text: qsTrId("session.close.stop")
                        checked: SessionStore.stopOnClose
                        onClicked: SessionStore.stopOnClose = checked
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTrId("session.close.backgroundHint")
                        color: Theme.muted
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Pane {
                Layout.fillWidth: true
                padding: 28
                palette.window: Theme.surface

                ColumnLayout {
                    width: parent.width
                    spacing: 16

                    Label {
                        text: qsTrId("settings.appearance")
                        font.weight: Font.Medium
                    }
                    AppearanceSelector {}
                }
            }

            Pane {
                Layout.fillWidth: true
                padding: 28
                palette.window: Theme.surface

                ColumnLayout {
                    width: parent.width
                    spacing: 16

                    Label {
                        text: qsTrId("settings.language")
                        font.weight: Font.Medium
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        Accessible.name: qsTrId("settings.language")
                        textRole: "name"
                        valueRole: "code"
                        model: [{ code: "auto", name: qsTrId("language.auto") }].concat(Array.from(LanguageManager.availableLanguages))
                        currentValue: LanguageManager.language
                        onActivated: LanguageManager.language = currentValue
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: LanguageManager.language === "auto"
                        text: qsTrId("language.autoHint")
                        color: Theme.muted
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}
