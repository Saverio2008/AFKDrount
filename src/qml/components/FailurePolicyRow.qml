import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root

    required property string title
    required property string policy
    property bool movement: false
    readonly property var actions: movement ? [qsTrId("config.policy.stop"), qsTrId("config.policy.ignore")] : [qsTrId("config.policy.stop"), qsTrId("config.policy.reconnect")]
    signal selected(string value)

    Label {
        Layout.fillWidth: true
        text: root.title
    }
    ComboBox {
        readonly property var scrollablePopup: popup.contentItem
        Component.onCompleted: scrollablePopup.boundsBehavior = Flickable.StopAtBounds
        Layout.preferredWidth: 180
        model: root.actions
        currentIndex: root.policy === "stop" ? 0 : 1
        Accessible.name: root.title
        onActivated: index => root.selected(index === 0 ? "stop" : root.movement ? "ignore" : "reconnect")
    }
}