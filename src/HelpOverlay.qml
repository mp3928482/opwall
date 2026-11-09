import QtQuick

// Every shortcut, shown with ?.
Rectangle {
    id: root
    signal closed()
    color: Qt.rgba(0, 0, 0, 0.72)

    MouseArea { anchors.fill: parent; onClicked: root.closed() }

    Rectangle {
        anchors.centerIn: parent
        width: Math.max(420, list.implicitWidth + 60)
        height: list.implicitHeight + 60
        radius: theme.radius
        color: theme.surface
        border.color: theme.accent

        Column {
            id: list
            anchors.centerIn: parent
            spacing: 12
            Text {
                text: qsTr("Keyboard shortcuts")
                color: theme.accent
                font.pixelSize: 16
                font.bold: true
            }
            Repeater {
                model: [
                    ["Space", qsTr("Pause or resume")],
                    ["R", qsTr("New random wall")],
                    ["F", qsTr("Toggle fullscreen")],
                    ["?", qsTr("Show or hide shortcuts")],
                    ["Esc", qsTr("Close / leave fullscreen")],
                    ["Q", qsTr("Quit")]
                ]
                delegate: Row {
                    required property var modelData
                    spacing: 16
                    Rectangle {
                        width: 64
                        height: 24
                        radius: Math.min(4, theme.radius)
                        color: "transparent"
                        border.color: theme.muted
                        Text {
                            anchors.centerIn: parent
                            text: modelData[0]
                            color: theme.foreground
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }
                    Text {
                        text: modelData[1]
                        color: theme.foreground
                        font.pixelSize: 13
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
            Text {
                text: qsTr("Seed %1").arg(backend.seed)
                color: theme.muted
                font.pixelSize: 11
            }
        }
    }
}
