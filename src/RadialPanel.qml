import QtQuick

// Panel 05: a segmented ring that counts up to its total, while a question
// types itself underneath.
Item {
    id: root
    property color tint: theme.accent

    readonly property int total: backend.radial.total || 40
    property int count: 0
    property int question: 0
    property int typed: 0
    readonly property string questionText: (backend.radial.questions || [""])[question % Math.max(1, (backend.radial.questions || []).length)]

    Connections { target: backend; function onRerolled() { root.count = 0; root.question = 0; root.typed = 0 } }

    Timer {
        interval: root.count >= root.total ? 2400 : 420
        repeat: true
        running: !backend.paused && root.visible
        onTriggered: {
            if (root.count >= root.total) {
                root.count = 0
                root.question += 1
                root.typed = 0
            } else {
                root.count += 1
            }
        }
    }
    Timer {
        interval: 45
        repeat: true
        running: !backend.paused && root.visible && root.typed < root.questionText.length
        onTriggered: root.typed += 1
    }

    // The ring's colors run through the theme palette.
    function segmentColor(i) {
        const p = theme.palette
        const span = [p[0], p[4], p[6], p[2], p[5], p[1]]
        const f = i / Math.max(1, root.total - 1) * (span.length - 1)
        const a = Qt.color(span[Math.floor(f)]), b = Qt.color(span[Math.min(span.length - 1, Math.ceil(f))])
        const k = f - Math.floor(f)
        return Qt.rgba(a.r + (b.r - a.r) * k, a.g + (b.g - a.g) * k, a.b + (b.b - a.b) * k, 1)
    }

    Item {
        id: dial
        anchors { left: parent.left; right: parent.right; top: parent.top; bottom: card.top; bottomMargin: 8 }

        Canvas {
            id: ring
            readonly property real size: Math.min(parent.width, parent.height)
            width: size
            height: size
            anchors.centerIn: parent
            property int count: root.count
            property var colorsKey: theme.palette
            onCountChanged: requestPaint()
            onColorsKeyChanged: requestPaint()
            onSizeChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                const c = size / 2, outer = size / 2 - 6, inner = outer - Math.max(10, size * 0.09)
                const n = root.total, gap = 0.035
                for (let i = 0; i < n; ++i) {
                    const a0 = -Math.PI / 2 + i / n * Math.PI * 2 + gap, a1 = -Math.PI / 2 + (i + 1) / n * Math.PI * 2 - gap
                    ctx.beginPath()
                    ctx.arc(c, c, outer, a0, a1, false)
                    ctx.arc(c, c, inner, a1, a0, true)
                    ctx.closePath()
                    ctx.fillStyle = i < root.count ? root.segmentColor(i) : "rgba(255,255,255,0.06)"
                    ctx.fill()
                }
                // A needle to the newest segment.
                const a = -Math.PI / 2 + (root.count - 0.5) / n * Math.PI * 2
                ctx.strokeStyle = theme.accent
                ctx.lineWidth = 1.5
                ctx.beginPath()
                ctx.moveTo(c, c)
                ctx.lineTo(c + Math.cos(a) * (inner - 4), c + Math.sin(a) * (inner - 4))
                ctx.stroke()
                ctx.fillStyle = theme.accent
                ctx.beginPath()
                ctx.arc(c, c, 3, 0, Math.PI * 2)
                ctx.fill()
            }
        }
        Column {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: ring.size * 0.12
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.count.toLocaleString(Qt.locale(translator.language), 'f', 0)
                color: root.count > 0 ? root.segmentColor(root.count - 1) : theme.foreground
                font.pixelSize: Math.max(18, ring.size * 0.16)
                font.bold: true
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("of %1").arg(root.total.toLocaleString(Qt.locale(translator.language), 'f', 0))
                color: theme.muted
                font.pixelSize: 10
            }
        }
    }

    Rectangle {
        id: card
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 52
        color: Qt.rgba(0, 0, 0, 0.3)
        radius: Math.min(4, theme.radius)
        border.color: Qt.rgba(1, 1, 1, 0.08)
        Column {
            anchors { fill: parent; margins: 8 }
            spacing: 4
            Text {
                text: "Q" + (root.count + 1 > root.total ? root.total : root.count + 1)
                color: root.tint
                font.pixelSize: 10
                font.bold: true
            }
            Text {
                width: parent.width
                text: root.questionText.substring(0, root.typed) + (root.typed < root.questionText.length ? "▌" : "")
                color: theme.foreground
                font.pixelSize: 10
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }
    }
}
