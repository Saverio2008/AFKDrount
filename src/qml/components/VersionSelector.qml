import QtQuick
import QtQuick.Controls

ComboBox {
    id: control
    readonly property var scrollablePopup: popup.contentItem
    Component.onCompleted: scrollablePopup.boundsBehavior = Flickable.StopAtBounds
    property string version: ""
    signal selected(string value)
    model: GameCatalog.versions
    textRole: "version"
    valueRole: "version"
    currentIndex: indexOfValue(version)
    displayText: currentIndex < 0 ? qsTrId("catalog.version.choose") : currentText
    Accessible.name: qsTrId("config.version")
    onActivated: index => control.selected(valueAt(index))
}