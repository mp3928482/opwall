import QtQuick

// Panel 04: an editor that writes its own code, next to a checklist that
// ticks itself off.
Item {
    id: root
    property color tint: theme.accent

    property var lines: []
    property int total: 0
    property int typed: 0
    property int checked: 0

    function prepare() {
        let offset = 0
        const result = []
        for (const tokens of backend.code) {
            const start = offset
            const prepared = tokens.map(t => {
                const item = { text: t.text, role: t.role, start: offset }
                offset += t.text.length
                return item
            })
            result.push({ tokens: prepared, start: start, length: offset - start })
            offset += 1 // the newline costs a keystroke too
        }
        lines = result
        total = offset
        typed = 0
    }
    Connections { target: backend; function onCodeChanged() { root.prepare() } }
    Connections { target: backend; function onRerolled() { root.checked = 0 } }
    Component.onCompleted: prepare()

    function roleColor(role) {
        const p = theme.palette
        return { keyword: p[2], string: p[3], number: p[4], type: p[0], function: p[1],
                 punctuation: theme.muted }[role] || theme.foreground
    }

    Timer {
        interval: 28
        repeat: true
        running: !backend.paused && root.visible && root.typed < root.total
        onTriggered: root.typed = Math.min(root.total, root.typed + 1 + (Math.random() < 0.35 ? 1 : 0))
    }
    Timer {
        interval: 2600
        running: !backend.paused && root.visible && root.total > 0 && root.typed >= root.total
        onTriggered: backend.nextCode()
    }
    Timer {
        interval: 1500
        repeat: true
        running: !backend.paused && root.visible
        onTriggered: root.checked = root.checked > (backend.doc.items || []).length ? 0 : root.checked + 1
    }

    Rectangle {
        id: editor
        width: parent.width * 0.6
        height: parent.height
        color: Qt.rgba(0, 0, 0, 0.25)
        radius: Math.min(4, theme.radius)
        border.color: Qt.rgba(1, 1, 1, 0.06)
        clip: true

        Text {
            id: tab
            anchors { left: parent.left; top: parent.top; margins: 8 }
            text: "main.ts"
            color: root.tint
            font.pixelSize: 10
        }
        Column {
            anchors { left: parent.left; top: tab.bottom; margins: 8 }
            spacing: 2
            Repeater {
                model: root.lines
                delegate: Row {
                    required property var modelData
                    required property int index
                    readonly property bool current: root.typed >= modelData.start
                                                    && root.typed <= modelData.start + modelData.length
                    height: 15
                    Text {
                        width: 24
                        text: index + 1
                        color: current ? theme.foreground : Qt.rgba(1, 1, 1, 0.2)
                        font.pixelSize: 10
                    }
                    Repeater {
                        model: modelData.tokens
                        delegate: Text {
                            required property var modelData
                            text: modelData.text.substring(0, Math.max(0, root.typed - modelData.start))
                            color: root.roleColor(modelData.role)
                            font.pixelSize: 11
                        }
                    }
                    Rectangle {
                        visible: parent.current
                        width: 7
                        height: 13
                        color: theme.accent
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            running: true
                            NumberAnimation { to: 0; duration: 450 }
                            NumberAnimation { to: 1; duration: 450 }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors { left: editor.right; leftMargin: 8; right: parent.right; top: parent.top; bottom: parent.bottom }
        color: Qt.rgba(0, 0, 0, 0.25)
        radius: Math.min(4, theme.radius)
        border.color: Qt.rgba(1, 1, 1, 0.06)
        clip: true

        Column {
            anchors { fill: parent; margins: 8 }
            spacing: 7
            Text { text: backend.doc.file || ""; color: theme.foreground; font.pixelSize: 10; bottomPadding: 4 }
            Repeater {
                model: backend.doc.items || []
                delegate: Row {
                    required property var modelData
                    required property int index
                    readonly property bool done: index < root.checked
                    spacing: 7
                    width: parent.width
                    Rectangle {
                        width: 11
                        height: 11
                        radius: Math.min(2, theme.radius)
                        color: done ? root.tint : "transparent"
                        border.color: done ? root.tint : theme.muted
                        Text {
                            anchors.centerIn: parent
                            visible: parent.parent.done
                            text: "✓"
                            color: "#101012"
                            font.pixelSize: 9
                            font.bold: true
                        }
                    }
                    Text {
                        width: parent.width - 18
                        text: modelData
                        color: done ? theme.muted : theme.foreground
                        font.pixelSize: 10
                        font.strikeout: done
                        elide: Text.ElideRight
                    }
                }
            }
            Repeater {
                model: 5
                delegate: Rectangle {
                    required property int index
                    width: parent.width * [0.9, 0.7, 0.82, 0.5, 0.65][index]
                    height: 4
                    radius: 2
                    color: Qt.rgba(1, 1, 1, 0.12)
                }
            }
        }
    }
}
