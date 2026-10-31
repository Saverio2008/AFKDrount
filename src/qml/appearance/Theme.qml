pragma Singleton

import QtQuick
import QtCore

QtObject {
    enum ColorMode {
        System,
        Light,
        Dark
    }

    property alias mode: preferences.colorMode
    readonly property Settings settings: Settings {
        id: preferences
        category: "appearance"
        property int colorMode: 0
    }
    readonly property bool dark: mode === Theme.Dark || (mode === Theme.System && Application.styleHints.colorScheme === Qt.Dark)

    function applyMode() {
        Application.styleHints.colorScheme = mode === Theme.Dark ? Qt.Dark : mode === Theme.Light ? Qt.Light : Qt.Unknown;
    }

    onModeChanged: {
        if (mode < Theme.System || mode > Theme.Dark)
            mode = Theme.System;
        else
            applyMode();
    }
    Component.onCompleted: {
        applyMode();
    }

    readonly property color canvas: dark ? "#222235" : "#eee8fb"
    readonly property color surface: dark ? "#20212a" : "#fffdf8"
    readonly property color accounts: dark ? "#2a2840" : "#f7f2ff"
    readonly property color instances: dark ? "#213a35" : "#e8f9f1"
    readonly property color sessions: dark ? "#2a273e" : "#f4efff"
    readonly property color field: dark ? "#312e43" : "#efedf5"
    readonly property color text: dark ? "#f2effb" : "#292442"
    readonly property color muted: dark ? "#bcb6d0" : "#6c637f"
    readonly property color error: dark ? "#ff9aa5" : "#a72536"
    readonly property color outline: dark ? "#49435f" : "#dfd7ec"
    readonly property color inputOutline: dark ? "#87799b" : "#897a9c"
    readonly property color primary: dark ? "#a18bea" : "#7053b8"
    readonly property color primaryText: dark ? "#1c1431" : "#ffffff"
    readonly property color selection: dark ? "#423660" : "#e4d9fa"
    readonly property color mintSelection: dark ? "#2b5549" : "#c9efdf"
    readonly property color mint: dark ? "#8ddfbb" : "#b9ebd2"
    readonly property color mintText: dark ? "#123b2a" : "#194431"
    readonly property color button: dark ? "#373148" : "#eae5f3"
    readonly property color disabledSurface: dark ? "#302d3d" : "#e9e5ef"
    readonly property color disabledText: dark ? "#8c849d" : "#92899f"
    readonly property color hoverTint: dark ? "#10ffffff" : "#09000000"
    readonly property color overlay: dark ? "#990e0c18" : "#66312a46"
    readonly property color running: dark ? "#78daa2" : "#217746"
    readonly property color queued: dark ? "#edbd65" : "#8b5e12"
    readonly property int panelRadius: 24
    readonly property int rowRadius: 16
    readonly property int fieldRadius: 22
    readonly property int fieldHeight: 44
    readonly property int buttonHeight: 40
    readonly property int panelPadding: 20
    readonly property int gutter: 12
}
