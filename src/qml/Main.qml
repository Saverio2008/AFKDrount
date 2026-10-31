import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AFKDrount
import AppAppearance

ApplicationWindow {
    id: window

    property bool settingsOpen: false
    property string monitoredSession: ""

    function closeMonitor() {
        monitoredSession = "";
    }

    width: 1200
    height: 800
    minimumWidth: 920
    minimumHeight: 680
    visible: true
    title: "AFK Drount"
    font.pointSize: 13
    color: Theme.canvas
    palette.window: Theme.canvas
    palette.windowText: Theme.text
    palette.base: Theme.field
    palette.text: Theme.text
    palette.button: Theme.button
    palette.buttonText: Theme.text
    palette.highlight: Theme.selection
    palette.highlightedText: Theme.text
    palette.placeholderText: Theme.muted
    palette.toolTipBase: Theme.surface
    palette.toolTipText: Theme.text
    palette.mid: Theme.outline
    palette.dark: Theme.outline
    palette.light: Theme.surface

    onClosing: close => {
        close.accepted = mainPage.requestClose();
        if (close.accepted) {
            if (SessionStore.activeCount > 0 && !SessionStore.stopOnClose) {
                close.accepted = false;
                window.hide();
            } else {
                Qt.quit();
            }
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: window.settingsOpen ? 1 : window.monitoredSession.length ? 2 : 0

        MainPage {
            id: mainPage

            onSettingsRequested: window.settingsOpen = true
            onMonitorRequested: sessionId => window.monitoredSession = sessionId
            onWindowCloseRequested: window.close()
        }
        Loader {
            readonly property var settingsPage: item
            active: window.settingsOpen
            sourceComponent: SettingsPage {}
            onLoaded: {
                settingsPage.backRequested.connect(() => { window.settingsOpen = false; });
            }
        }
        Loader {
            id: monitorLoader
            readonly property var monitorPage: item
            active: window.monitoredSession.length > 0
            sourceComponent: MonitorPage {}
            onLoaded: {
                monitorPage.sessionId = window.monitoredSession;
                monitorPage.backRequested.connect(window.closeMonitor);
            }
        }
    }
}
