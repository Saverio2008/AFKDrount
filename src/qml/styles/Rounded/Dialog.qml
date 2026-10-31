import QtQuick
import QtQuick.Templates as T
import AppAppearance

T.Dialog {
    id: control

    implicitWidth: Math.max(implicitContentWidth + leftPadding + rightPadding, implicitHeaderWidth, implicitFooterWidth)
    implicitHeight: implicitContentHeight + topPadding + bottomPadding + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0) + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0)
    padding: 24
    spacing: 12

    background: Rectangle {
        color: Theme.surface
        radius: Theme.panelRadius
        border.color: Theme.outline
    }
    header: Item {
        visible: control.title.length > 0
        implicitWidth: titleLabel.implicitWidth
        implicitHeight: visible ? Math.ceil(Math.max(titleMetrics.height, titleMetrics.boundingRect(control.title).height)) + titleLabel.topPadding + titleLabel.bottomPadding : 0

        FontMetrics {
            id: titleMetrics
            font: titleLabel.font
        }
        Label {
            id: titleLabel
            anchors.fill: parent
            text: control.title
            font.weight: Font.Medium
            font.pointSize: 16
            padding: 24
            bottomPadding: 0
            elide: Text.ElideRight
        }
    }
    footer: DialogButtonBox {
        visible: count > 0
        standardButtons: control.standardButtons
    }
    T.Overlay.modal: Rectangle {
        color: Theme.overlay
    }
}