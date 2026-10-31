import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    required property string title
    required property var region
    property bool allowUnlimitedY: false
    readonly property var draft: ({
            from: [coordinate(fromX.text), unlimited.checked ? null : coordinate(fromY.text), coordinate(fromZ.text)],
            to: [coordinate(toX.text), unlimited.checked ? null : coordinate(toY.text), coordinate(toZ.text)],
            unlimitedY: unlimited.checked
        })

    //noinspection JSUnusedGlobalSymbols
    function coordinate(value) {
        return value.trim().length > 0 ? +value : NaN;
    }

    //noinspection JSUnusedGlobalSymbols
    function reset() {
        fromX.text = String(region.from[0]);
        fromY.text = region.from[1] === null ? "" : String(region.from[1]);
        fromZ.text = String(region.from[2]);
        toX.text = String(region.to[0]);
        toY.text = region.to[1] === null ? "" : String(region.to[1]);
        toZ.text = String(region.to[2]);
        unlimited.checked = allowUnlimitedY && region.unlimitedY === true;
    }

    property bool ready: false
    onRegionChanged: {
        if (root.ready)
            root.reset();
    }
    Component.onCompleted: {
        root.ready = true;
        root.reset();
    }
    spacing: 6

    RowLayout {
        Layout.fillWidth: true
        Label {
            Layout.fillWidth: true
            text: root.title
            font.weight: Font.Medium
        }
        CheckBox {
            id: unlimited
            visible: root.allowUnlimitedY
            text: qsTrId("config.region.unlimitedY")
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 4
        columnSpacing: 6
        rowSpacing: 6

        Item {
            implicitWidth: 1
            implicitHeight: 1
        }
        Label {
            text: "X"
        }
        Label {
            text: "Y"
        }
        Label {
            text: "Z"
        }
        Label {
            text: qsTrId("config.region.from")
        }
        CompactTextField {
            id: fromX
            placeholderText: "X"
            Layout.fillWidth: true
            Layout.preferredWidth: 90
            Accessible.name: qsTrId("config.region.coordinate").arg(root.title).arg(qsTrId("config.region.from")).arg("X")
        }
        CompactTextField {
            id: fromY
            placeholderText: enabled ? "Y" : ""
            Layout.fillWidth: true
            Layout.preferredWidth: 90
            enabled: !unlimited.checked
            Accessible.name: qsTrId("config.region.coordinate").arg(root.title).arg(qsTrId("config.region.from")).arg("Y")
        }
        CompactTextField {
            id: fromZ
            placeholderText: "Z"
            Layout.fillWidth: true
            Layout.preferredWidth: 90
            Accessible.name: qsTrId("config.region.coordinate").arg(root.title).arg(qsTrId("config.region.from")).arg("Z")
        }
        Label {
            text: qsTrId("config.region.to")
        }
        CompactTextField {
            id: toX
            placeholderText: "X"
            Layout.fillWidth: true
            Layout.preferredWidth: 90
            Accessible.name: qsTrId("config.region.coordinate").arg(root.title).arg(qsTrId("config.region.to")).arg("X")
        }
        CompactTextField {
            id: toY
            placeholderText: enabled ? "Y" : ""
            Layout.fillWidth: true
            Layout.preferredWidth: 90
            enabled: !unlimited.checked
            Accessible.name: qsTrId("config.region.coordinate").arg(root.title).arg(qsTrId("config.region.to")).arg("Y")
        }
        CompactTextField {
            id: toZ
            placeholderText: "Z"
            Layout.fillWidth: true
            Layout.preferredWidth: 90
            Accessible.name: qsTrId("config.region.coordinate").arg(root.title).arg(qsTrId("config.region.to")).arg("Z")
        }
    }
}
