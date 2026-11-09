import QtQuick

// Panel 02: a node canvas where a ghost cursor keeps rearranging the boxes,
// with the file's JSON underneath following along.
Item {
    id: root
    property color tint: theme.accent

    // Box positions as fractions of the board, and the drag in progress.
    property var pos: []
    property int dragged: -1
    property real cx: 0.5
    property real cy: 0.5

    // phase 0: glide to a box; 1: drag it; 2: rest.
    property int phase: 2
    property real t: 0
    property var from: ({})
    property var to: ({})

    function reset() {
        pos = (backend.canvas.boxes || []).map(b => ({ x: b.x, y: b.y }))
        dragged = -1
        phase = 2
        t = 0.6
    }
    Connections { target: backend; function onRerolled() { root.reset() } }
    Component.onCompleted: reset()

    function ease(v) { return v < 0.5 ? 4 * v * v * v : 1 - Math.pow(-2 * v + 2, 3) / 2 }

    FrameAnimation {
        running: !backend.paused && root.visible
        onTriggered: {
            const boxes = root.pos
            if (boxes.length === 0)
                return
            root.t += frameTime / (root.phase === 2 ? 0.7 : root.phase === 0 ? 0.6 : 1.1)
            const k = root.ease(Math.min(1, root.t))
            if (root.phase === 0) {
                root.cx = root.from.x + (root.to.x - root.from.x) * k
                root.cy = root.from.y + (root.to.y - root.from.y) * k
            } else if (root.phase === 1) {
                const next = boxes.slice()
                next[root.dragged] = { x: root.from.bx + (root.to.bx - root.from.bx) * k,
                                       y: root.from.by + (root.to.by - root.from.by) * k }
                root.cx = next[root.dragged].x + root.grab.x
                root.cy = next[root.dragged].y + root.grab.y
                root.pos = next
            }
            if (root.t < 1)
                return
            root.t = 0
            if (root.phase === 2) {
                // Pick a box and a spot to drag it to.
                const i = Math.floor(Math.random() * boxes.length)
                root.dragged = i
                root.grab = { x: 0.04 + Math.random() * 0.14, y: 0.05 + Math.random() * 0.06 }
                root.from = { x: root.cx, y: root.cy }
                root.to = { x: boxes[i].x + root.grab.x, y: boxes[i].y + root.grab.y }
                root.phase = 0
            } else if (root.phase === 0) {
                const i = root.dragged
                root.from = { bx: boxes[i].x, by: boxes[i].y }
                root.to = { bx: Math.max(0.01, Math.min(0.74, boxes[i].x + (Math.random() - 0.5) * 0.45)),
                            by: Math.max(0.02, Math.min(0.78, boxes[i].y + (Math.random() - 0.5) * 0.5)) }
                root.phase = 1
            } else {
                root.phase = 2
                root.dragged = -1
            }
        }
    }
    property var grab: ({ x: 0.1, y: 0.05 })

    Rectangle {
        id: board
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: parent.height * 0.6
        color: Qt.rgba(0, 0, 0, 0.25)
        radius: Math.min(4, theme.radius)
        border.color: Qt.rgba(1, 1, 1, 0.06)
        clip: true

        readonly property real boxW: Math.min(120, width * 0.26)
        readonly property real boxH: 28

        // A dotted grid, drawn once per size.
        Canvas {
            anchors.fill: parent
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = "rgba(255,255,255,0.08)"
                for (let x = 8; x < width; x += 16)
                    for (let y = 8; y < height; y += 16)
                        ctx.fillRect(x, y, 1, 1)
            }
        }

        Canvas {
            id: wires
            anchors.fill: parent
            property var positions: root.pos
            onPositionsChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.lineWidth = 1.5
                ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.35)
                for (const edge of backend.canvas.edges || []) {
                    const a = root.pos[edge[0]], b = root.pos[edge[1]]
                    if (!a || !b) continue
                    ctx.beginPath()
                    const ax = a.x * width + board.boxW / 2, ay = a.y * height + board.boxH / 2
                    const bx = b.x * width + board.boxW / 2, by = b.y * height + board.boxH / 2
                    ctx.moveTo(ax, ay)
                    ctx.bezierCurveTo((ax + bx) / 2, ay, (ax + bx) / 2, by, bx, by)
                    ctx.stroke()
                }
            }
        }

        Repeater {
            model: backend.canvas.boxes || []
            delegate: Rectangle {
                required property var modelData
                required property int index
                readonly property color tint: theme.palette[modelData.color % theme.palette.length]
                readonly property bool active: root.dragged === index && root.phase === 1
                x: (root.pos[index] || modelData).x * board.width
                y: (root.pos[index] || modelData).y * board.height
                width: board.boxW
                height: board.boxH
                radius: Math.min(4, theme.radius)
                color: Qt.rgba(tint.r, tint.g, tint.b, active ? 0.32 : 0.16)
                border.color: active ? "white" : tint
                border.width: active ? 1.5 : 1
                Rectangle { width: 3; height: parent.height; color: parent.tint; radius: parent.radius }
                Column {
                    anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                    spacing: 3
                    Text { text: modelData.label; color: theme.foreground; font.pixelSize: 10; font.bold: true }
                }
                Rectangle {
                    anchors { right: parent.right; rightMargin: 8; verticalCenter: parent.verticalCenter }
                    width: parent.width * 0.25
                    height: 3
                    radius: 1
                    color: Qt.rgba(1, 1, 1, 0.18)
                }
            }
        }

        // The ghost cursor.
        Canvas {
            width: 14
            height: 20
            x: root.cx * board.width
            y: root.cy * board.height
            onPaint: {
                const ctx = getContext("2d")
                ctx.fillStyle = "white"
                ctx.strokeStyle = "black"
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(1, 1); ctx.lineTo(1, 16); ctx.lineTo(5, 12); ctx.lineTo(8, 19)
                ctx.lineTo(10, 18); ctx.lineTo(7, 11); ctx.lineTo(12, 11); ctx.closePath()
                ctx.fill(); ctx.stroke()
            }
        }
    }

    Rectangle {
        anchors { left: parent.left; right: parent.right; top: board.bottom; topMargin: 8; bottom: parent.bottom }
        color: Qt.rgba(0, 0, 0, 0.25)
        radius: Math.min(4, theme.radius)
        border.color: Qt.rgba(1, 1, 1, 0.06)
        clip: true

        Text {
            id: fileLabel
            anchors { left: parent.left; top: parent.top; margins: 8 }
            text: (backend.canvas.file || "") + "  ·  " + qsTr("plain text, yours to keep")
            color: root.tint
            font.pixelSize: 10
        }
        Text {
            anchors { left: parent.left; right: parent.right; top: fileLabel.bottom; margins: 8 }
            textFormat: Text.StyledText
            color: theme.foreground
            font.pixelSize: 10
            lineHeight: 1.15
            text: {
                const boxes = backend.canvas.boxes || []
                const lines = ['{ <font color="' + theme.palette[3] + '">"nodes"</font>: [']
                for (let i = 0; i < boxes.length; ++i) {
                    const p = root.pos[i] || boxes[i]
                    const hot = i === root.dragged
                    const num = c => '<font color="' + (hot ? theme.accent : theme.palette[0]) + '">' + c + '</font>'
                    lines.push('&nbsp;&nbsp;{ "id": <font color="' + theme.palette[1] + '">"' + boxes[i].label.toLowerCase()
                               + '"</font>, "x": ' + num(Math.round(p.x * 1000)) + ', "y": ' + num(Math.round(p.y * 1000)) + ' }'
                               + (i < boxes.length - 1 ? "," : ""))
                }
                lines.push('], "edges": ' + (backend.canvas.edges || []).length + ' }')
                return lines.join("<br>")
            }
        }
    }
}
