import QtQuick

// Panel 06: tracks of colored spans with a playhead sweeping across them.
Item {
    id: root
    property color tint: theme.accent
    property real head: 0

    NumberAnimation on head {
        id: sweep
        from: 0
        to: 1
        duration: 9000
        loops: Animation.Infinite
        running: true
        paused: backend.paused || !root.visible
    }

    Rectangle {
        id: track
        anchors { left: parent.left; right: parent.right; top: parent.top; bottom: footer.top; bottomMargin: 6 }
        color: Qt.rgba(0, 0, 0, 0.25)
        radius: Math.min(4, theme.radius)
        border.color: Qt.rgba(1, 1, 1, 0.06)
        clip: true

        readonly property int rows: backend.timeline.rows || 10
        readonly property real rowHeight: (height - 16) / rows

        Repeater {
            model: track.rows
            delegate: Rectangle {
                required property int index
                x: 0
                y: 8 + index * track.rowHeight + track.rowHeight - 1
                width: track.width
                height: 1
                color: Qt.rgba(1, 1, 1, 0.04)
            }
        }
        Repeater {
            model: backend.timeline.blocks || []
            delegate: Rectangle {
                required property var modelData
                readonly property color tint: theme.palette[modelData.color % theme.palette.length]
                readonly property bool hit: root.head >= modelData.start && root.head <= modelData.start + modelData.length
                x: 6 + modelData.start * (track.width - 12)
                y: 8 + modelData.row * track.rowHeight + 2
                width: modelData.length * (track.width - 12)
                height: track.rowHeight - 5
                radius: Math.min(3, theme.radius)
                color: Qt.rgba(tint.r, tint.g, tint.b, hit ? 0.95 : 0.5)
                border.color: hit ? "white" : "transparent"
                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }
        Rectangle {
            x: 6 + root.head * (track.width - 12)
            width: 2
            height: track.height
            color: root.tint
            Rectangle {
                width: 12
                height: 12
                radius: 6
                anchors.horizontalCenter: parent.horizontalCenter
                y: track.height / 2 - 6
                color: theme.surface
                border.color: root.tint
                border.width: 2
            }
        }
    }

    Item {
        id: footer
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 14
        Text { anchors.left: parent.left; text: qsTr("before"); color: root.tint; font.pixelSize: 10 }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            readonly property real seconds: root.head * 90
            text: Math.floor(seconds / 60).toString().padStart(2, "0") + ":"
                  + (seconds % 60).toLocaleString(Qt.locale(translator.language), 'f', 2).padStart(5, "0")
            color: theme.muted
            font.pixelSize: 10
        }
        Text { anchors.right: parent.right; text: qsTr("after"); color: theme.muted; font.pixelSize: 10 }
    }
}
