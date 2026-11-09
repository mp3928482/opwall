import QtQuick

// The crawl along the bottom of the wall.
Rectangle {
    id: root
    color: theme.surface
    clip: true

    readonly property string line: backend.ticker.join("   ✦   ") + "   ✦   "

    Rectangle { width: parent.width; height: 1; color: Qt.rgba(1, 1, 1, 0.08) }

    Row {
        id: strip
        anchors.verticalCenter: parent.verticalCenter
        Repeater {
            model: 2
            delegate: Text {
                id: copy
                text: root.line
                color: theme.muted
                font.pixelSize: 11
            }
        }
        NumberAnimation on x {
            id: crawl
            from: 0
            to: -strip.width / 2
            duration: Math.max(1, strip.width / 2 / 60 * 1000)
            loops: Animation.Infinite
            running: true
            paused: backend.paused
        }
    }
    Connections { target: backend; function onRerolled() { crawl.restart() } }
}
