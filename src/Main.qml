import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

ApplicationWindow {
    id: win
    width: 1600
    height: 940
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    visibility: startFullscreen ? Window.FullScreen : Window.AutomaticVisibility
    title: "Opwall"
    color: theme.background
    font.family: "monospace"

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool helpVisible: false
    readonly property bool tall: wall.height > wall.width * 1.05

    Shortcut { sequences: ["?", "Shift+?"]; context: Qt.ApplicationShortcut; onActivated: win.helpVisible = !win.helpVisible }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; onActivated: backend.togglePaused() }
    Shortcut { sequence: "R"; context: Qt.ApplicationShortcut; onActivated: backend.reroll() }
    Shortcut {
        sequence: "F"
        context: Qt.ApplicationShortcut
        onActivated: win.visibility = win.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen
    }
    Shortcut {
        sequence: "Esc"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (win.helpVisible)
                win.helpVisible = false
            else if (win.visibility === Window.FullScreen)
                win.visibility = Window.Windowed
        }
    }

    // Each panel's place as fractions of the wall: a wide grid for a wide
    // window and a stacked one, like a phone video, for a tall one.
    readonly property var wideLayout: [
        [0, 0, .5, .55], [.5, 0, .24, .55], [.74, 0, .26, .55],
        [0, .55, .3, .45], [.3, .55, .17, .45], [.47, .55, .27, .45], [.74, .55, .26, .45]
    ]
    readonly property var tallLayout: [
        [0, 0, .64, .4], [.64, 0, .36, .4], [0, .4, .5, .3],
        [.5, .4, .5, .3], [0, .7, .33, .3], [.33, .7, .34, .3], [.67, .7, .33, .3]
    ]
    function place(index) {
        return (tall ? tallLayout : wideLayout)[index]
    }
    function panelInfo(index) {
        return backend.panels[index] || ({ title: "", subtitle: "", tag: "" })
    }
    function tint(index) {
        return theme.palette[[5, 1, 3, 4, 0, 2, 6][index] % theme.palette.length]
    }

    // Right-to-left languages mirror the window; the Translator sets the
    // application's layout direction.
    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        Header {
            id: header
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
            height: 66
        }

        Item {
            id: wall
            anchors { left: parent.left; right: parent.right; top: header.bottom; bottom: ticker.top
                      leftMargin: 8; rightMargin: 8; topMargin: 8; bottomMargin: 4 }

            component Slot: Panel {
                required property int slot
                readonly property var r: win.place(slot)
                readonly property int gap: 10
                x: r[0] * wall.width + gap / 2
                y: r[1] * wall.height + gap / 2
                width: r[2] * wall.width - gap
                height: r[3] * wall.height - gap
                number: slot + 1
                title: win.panelInfo(slot).title
                subtitle: win.panelInfo(slot).subtitle
                tag: win.panelInfo(slot).tag
                tint: win.tint(slot)
                Behavior on x { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
                Behavior on y { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
            }

            Slot { slot: 0; GraphPanel { anchors.fill: parent } }
            Slot { slot: 1; CanvasPanel { anchors.fill: parent; tint: win.tint(1) } }
            Slot { slot: 2; RegistryPanel { anchors.fill: parent; tint: win.tint(2) } }
            Slot { slot: 3; CodePanel { anchors.fill: parent; tint: win.tint(3) } }
            Slot { slot: 4; RadialPanel { anchors.fill: parent; tint: win.tint(4) } }
            Slot { slot: 5; TimelinePanel { anchors.fill: parent; tint: win.tint(5) } }
            Slot { slot: 6; PalettePanel { anchors.fill: parent; tint: win.tint(6) } }
        }

        Ticker {
            id: ticker
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: 26
        }

        HelpOverlay {
            anchors.fill: parent
            visible: win.helpVisible
            onClosed: win.helpVisible = false
        }
    }
}
