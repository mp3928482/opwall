import QtQuick

// Panel 03: a ranked list whose numbers keep moving, beside a grid of
// cards with one spotlit at a time.
Item {
    id: root
    property color tint: theme.accent
    property var values: []
    property int spotlight: 0

    function reset() { values = (backend.registry.rows || []).map(r => r.value) }
    Connections { target: backend; function onRerolled() { root.reset() } }
    Component.onCompleted: reset()

    readonly property real maxValue: Math.max(1, ...values)

    Timer {
        interval: 450
        repeat: true
        running: !backend.paused && root.visible
        onTriggered: {
            const next = root.values.slice()
            const i = Math.floor(Math.random() * next.length)
            next[i] = Math.max(100000, Math.round(next[i] * (0.9 + Math.random() * 0.22)))
            root.values = next
        }
    }
    Timer {
        interval: 1300
        repeat: true
        running: !backend.paused && root.visible
        onTriggered: root.spotlight = (root.spotlight + 1) % Math.max(1, (backend.registry.cards || []).length)
    }

    function short(v) {
        return (v / 1e6).toLocaleString(Qt.locale(translator.language), 'f', 2) + "M"
    }

    Column {
        id: list
        width: parent.width * 0.55
        spacing: 4
        Text { text: backend.registry.rowsFile || ""; color: root.tint; font.pixelSize: 10; bottomPadding: 4 }
        Repeater {
            model: backend.registry.rows || []
            delegate: Row {
                required property var modelData
                required property int index
                width: list.width
                height: Math.max(12, (root.height - 30) / 15)
                spacing: 6
                Text {
                    width: parent.width * 0.38
                    text: modelData.name
                    color: theme.foreground
                    font.pixelSize: 10
                    elide: Text.ElideRight
                    anchors.verticalCenter: parent.verticalCenter
                }
                Item {
                    width: parent.width * 0.42
                    height: 5
                    anchors.verticalCenter: parent.verticalCenter
                    Rectangle { anchors.fill: parent; radius: 2; color: Qt.rgba(1, 1, 1, 0.05) }
                    Rectangle {
                        height: parent.height
                        radius: 2
                        color: root.tint
                        opacity: 0.85
                        width: parent.width * (root.values[index] || 0) / root.maxValue
                        Behavior on width { NumberAnimation { duration: 400; easing.type: Easing.OutCubic } }
                    }
                }
                Text {
                    width: parent.width * 0.2 - 12
                    horizontalAlignment: Text.AlignRight
                    text: root.short(root.values[index] || 0)
                    color: theme.muted
                    font.pixelSize: 10
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    Item {
        anchors { left: list.right; leftMargin: 12; right: parent.right; top: parent.top; bottom: parent.bottom }
        Text { id: cardsLabel; text: backend.registry.cardsFile || ""; color: theme.palette[6]; font.pixelSize: 10 }
        Grid {
            id: grid
            anchors { top: cardsLabel.bottom; topMargin: 8; left: parent.left; right: parent.right; bottom: parent.bottom }
            columns: 2
            spacing: 6
            Repeater {
                model: backend.registry.cards || []
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    readonly property color tint: theme.palette[modelData.color % theme.palette.length]
                    readonly property bool lit: root.spotlight === index
                    width: (grid.width - grid.spacing) / 2
                    height: (grid.height - grid.spacing * 3) / 4
                    radius: Math.min(4, theme.radius)
                    color: lit ? Qt.rgba(tint.r, tint.g, tint.b, 0.16) : Qt.rgba(1, 1, 1, 0.03)
                    border.color: lit ? tint : Qt.rgba(1, 1, 1, 0.08)
                    Behavior on color { ColorAnimation { duration: 300 } }
                    Column {
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 7 }
                        spacing: 4
                        Repeater {
                            model: modelData.lines
                            delegate: Rectangle {
                                required property int index
                                width: parent.width * [0.8, 0.55, 0.7, 0.4][index % 4]
                                height: 3
                                radius: 1
                                color: index === 0 ? tint : Qt.rgba(1, 1, 1, 0.16)
                            }
                        }
                    }
                    Text {
                        anchors { left: parent.left; bottom: parent.bottom; margins: 6 }
                        width: parent.width - 12
                        text: modelData.title
                        color: lit ? theme.foreground : theme.muted
                        font.pixelSize: 9
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
