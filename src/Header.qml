import QtQuick

// The top strip: the operation's name block, its title, and live counters.
Item {
    id: root

    readonly property var info: backend.header
    property var values: []

    function resetValues() {
        values = (info.stats || []).map(s => s.value)
    }
    Connections { target: backend; function onRerolled() { root.resetValues() } }
    Component.onCompleted: resetValues()

    // The counters only ever climb, a little at a time, like a live feed.
    Timer {
        interval: 900
        repeat: true
        running: !backend.paused
        onTriggered: {
            const next = root.values.slice()
            for (let i = 1; i < next.length; ++i)
                if (Math.random() < 0.6)
                    next[i] += Math.floor(Math.random() * (i === 1 ? 14 : 3))
            root.values = next
        }
    }

    Rectangle {
        id: org
        width: Math.max(200, orgName.implicitWidth + 40)
        height: parent.height
        radius: theme.radius
        color: theme.accent
        Column {
            anchors.centerIn: parent
            spacing: 4
            Text {
                id: orgName
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.info.org || ""
                color: theme.accentForeground
                font.pixelSize: 20
                font.bold: true
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("%n live feeds", "", root.values[0] || 0)
                color: theme.accentForeground
                opacity: 0.75
                font.pixelSize: 10
            }
        }
    }

    Column {
        anchors { left: org.right; leftMargin: 18; right: stats.left; rightMargin: 18; verticalCenter: parent.verticalCenter }
        spacing: 8
        Row {
            spacing: 14
            Text {
                text: root.info.title || ""
                color: theme.foreground
                font.pixelSize: 16
                font.bold: true
            }
            Text {
                visible: backend.paused
                text: "▌▌ " + qsTr("Paused")
                color: theme.accent
                font.pixelSize: 13
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
                SequentialAnimation on opacity {
                    running: backend.paused
                    loops: Animation.Infinite
                    NumberAnimation { from: 1; to: 0.3; duration: 600 }
                    NumberAnimation { from: 0.3; to: 1; duration: 600 }
                }
            }
        }
        Text {
            width: parent.width
            text: root.info.subtitle || ""
            color: theme.muted
            font.pixelSize: 11
            elide: Text.ElideRight
        }
    }

    Row {
        id: stats
        anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
        spacing: 34
        Repeater {
            model: root.info.stats || []
            delegate: Column {
                required property var modelData
                required property int index
                spacing: 4
                Text {
                    text: modelData.label.toUpperCase()
                    color: theme.muted
                    font.pixelSize: 9
                    font.letterSpacing: 1
                }
                Text {
                    text: (root.values[index] || 0).toLocaleString(Qt.locale(translator.language), 'f', 0)
                    color: theme.palette[modelData.color % theme.palette.length]
                    font.pixelSize: 24
                    font.bold: true
                }
            }
        }
    }
}
