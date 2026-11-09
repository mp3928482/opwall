import QtQuick

// Panel 07: the theme's own colors as swatches, and a page that writes
// itself, flipping between light and dark paper.
Item {
    id: root
    property color tint: theme.accent

    readonly property var keys: ["red", "orange", "yellow", "green", "cyan", "blue", "magenta"]
    property int picked: 0
    property bool light: false

    property var lines: []
    property int total: 0
    property int typed: 0

    function prepare() {
        let offset = 0
        lines = backend.writer.map(l => {
            const item = { text: l.text, role: l.role, start: offset }
            offset += l.text.length + 1
            return item
        })
        total = offset
        typed = 0
    }
    Connections { target: backend; function onWriterChanged() { root.prepare() } }
    Component.onCompleted: prepare()

    Timer {
        interval: 1100
        repeat: true
        running: !backend.paused && root.visible
        onTriggered: root.picked = (root.picked + 1) % root.keys.length
    }
    Timer {
        interval: 40
        repeat: true
        running: !backend.paused && root.visible && root.typed < root.total
        onTriggered: root.typed += 1
    }
    Timer {
        interval: 2400
        running: !backend.paused && root.visible && root.total > 0 && root.typed >= root.total
        onTriggered: {
            root.light = !root.light
            backend.nextWriter()
        }
    }

    function ink(role) {
        const c = theme.colors
        const pick = { heading: c.orange, list: page.text, quote: theme.muted, code: c.cyan }[role] || page.text
        return light && role !== "list" && role !== "plain" ? Qt.darker(pick, 1.6) : pick
    }

    Row {
        id: swatches
        width: parent.width
        spacing: 5
        Repeater {
            model: root.keys
            delegate: Rectangle {
                required property string modelData
                required property int index
                width: (swatches.width - swatches.spacing * (root.keys.length - 1)) / root.keys.length
                height: 34
                radius: Math.min(4, theme.radius)
                color: theme.colors[modelData] || "#888"
                border.color: "white"
                border.width: root.picked === index ? 2 : 0
                scale: root.picked === index ? 1.06 : 1
                Behavior on scale { NumberAnimation { duration: 160 } }
            }
        }
    }
    Row {
        id: readout
        anchors { top: swatches.bottom; topMargin: 6 }
        spacing: 8
        Text {
            text: (theme.colors[root.keys[root.picked]] || "").toUpperCase()
            color: theme.colors[root.keys[root.picked]] || theme.foreground
            font.pixelSize: 10
            font.bold: true
        }
        Text { text: root.keys[root.picked]; color: theme.muted; font.pixelSize: 10 }
    }
    Text {
        anchors { right: parent.right; verticalCenter: readout.verticalCenter }
        text: root.light ? qsTr("light") : qsTr("dark")
        color: root.tint
        font.pixelSize: 10
    }

    Rectangle {
        id: page
        readonly property color text: root.light ? theme.surface : theme.foreground
        anchors { left: parent.left; right: parent.right; top: readout.bottom; topMargin: 8; bottom: parent.bottom }
        radius: Math.min(4, theme.radius)
        color: root.light ? theme.foreground : Qt.rgba(0, 0, 0, 0.3)
        border.color: Qt.rgba(1, 1, 1, 0.08)
        clip: true
        Behavior on color { ColorAnimation { duration: 500 } }

        Column {
            anchors { fill: parent; margins: 10 }
            spacing: 3
            Repeater {
                model: root.lines
                delegate: Text {
                    required property var modelData
                    width: parent.width
                    text: modelData.text.substring(0, Math.max(0, root.typed - modelData.start))
                    color: root.ink(modelData.role)
                    font.pixelSize: modelData.role === "heading" ? 13 : 10
                    font.bold: modelData.role === "heading"
                    font.italic: modelData.role === "quote"
                    elide: Text.ElideRight
                }
            }
        }
    }
}
