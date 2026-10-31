import QtQuick
import QtQuick.Controls

TextField {
    topPadding: 4
    bottomPadding: 4
    implicitHeight: Math.max(28, contentHeight + topPadding + bottomPadding)
    selectByMouse: true
}