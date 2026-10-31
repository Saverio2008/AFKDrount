import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog

    required property bool canSave
    property string errorText: ""

    signal saveRequested
    signal discardRequested

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent.width - 48, 500)
    title: qsTrId("unsaved.title")
    modal: true
    closePolicy: Popup.CloseOnEscape

    ColumnLayout {
        width: parent.width
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: qsTrId("unsaved.description")
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            visible: !dialog.canSave
            text: qsTrId("unsaved.invalidHint")
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            visible: dialog.errorText.length > 0
            text: dialog.errorText
            wrapMode: Text.WordWrap
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTrId("action.cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            text: qsTrId("action.discard")
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: dialog.discardRequested()
        }
        Button {
            text: qsTrId("action.saveAndContinue")
            enabled: dialog.canSave
            DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
            onClicked: dialog.saveRequested()
        }
    }
}