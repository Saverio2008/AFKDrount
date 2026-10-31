import QtQuick
import QtQuick.Templates as T

T.DialogButtonBox {
    id: control

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: implicitContentHeight + topPadding + bottomPadding
    contentWidth: buttons.contentWidth
    spacing: 10
    padding: 20
    alignment: Qt.AlignRight

    delegate: Button {}

    contentItem: ListView {
        id: buttons

        implicitWidth: contentWidth
        model: control.contentModel
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
    }
}