import QtQuick
import Opwall

// Panel 01: the vault graph, with a legend and a readout.
Item {
    id: root

    NodeGraph {
        id: graph
        anchors.fill: parent
        model: backend.graph
        colors: theme.palette
        accent: theme.accent
        running: !backend.paused && root.visible
    }

    // Hub labels as pills in their cluster's color, riding on the hubs.
    Repeater {
        model: backend.graph.clusters
        delegate: Rectangle {
            required property var modelData
            required property int index
            readonly property color tint: theme.palette[index % theme.palette.length]
            readonly property point at: graph.hubPoints[index] || Qt.point(0, 0)
            x: at.x - width / 2
            y: at.y - height - 9
            width: label.implicitWidth + 14
            height: label.implicitHeight + 6
            radius: height / 2
            color: Qt.rgba(0, 0, 0, 0.7)
            border.color: tint
            Text {
                id: label
                anchors.centerIn: parent
                text: modelData.name
                color: Qt.lighter(parent.tint, 1.3)
                font.pixelSize: 10
            }
        }
    }

    // The root: a lettered disc in the accent, its name beneath.
    Rectangle {
        id: rootDisc
        width: 22
        height: 22
        radius: 11
        x: graph.width / 2 - 11
        y: graph.height / 2 - 11
        color: Qt.darker(theme.accent, 1.5)
        border.color: theme.accent
        border.width: 1.5
        Text {
            anchors.centerIn: parent
            text: backend.graph.rootLetter || ""
            color: "white"
            font.pixelSize: 12
            font.bold: true
        }
    }
    Text {
        anchors { horizontalCenter: rootDisc.horizontalCenter; top: rootDisc.bottom; topMargin: 5 }
        text: backend.graph.root || ""
        color: Qt.rgba(1, 1, 1, 0.8)
        font.pixelSize: 10
    }

    Rectangle {
        anchors { left: parent.left; top: parent.top; margins: 4 }
        width: legend.implicitWidth + 20
        height: legend.implicitHeight + 16
        radius: Math.min(4, theme.radius)
        color: Qt.rgba(0, 0, 0, 0.45)
        border.color: Qt.rgba(1, 1, 1, 0.08)
        Column {
            id: legend
            anchors.centerIn: parent
            spacing: 5
            Repeater {
                model: backend.graph.clusters
                delegate: Row {
                    required property var modelData
                    required property int index
                    spacing: 8
                    readonly property color tint: theme.palette[index % theme.palette.length]
                    Rectangle { width: 8; height: 8; radius: 4; color: tint; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: 110; text: modelData.name + "/"; color: theme.foreground; font.pixelSize: 10 }
                    Text {
                        text: modelData.count.toLocaleString(Qt.locale(translator.language), 'f', 0)
                        color: tint
                        font.pixelSize: 10
                    }
                }
            }
        }
    }

    Text {
        anchors { right: parent.right; bottom: parent.bottom; margins: 4 }
        color: theme.muted
        font.pixelSize: 10
        text: qsTr("zoom %1%").arg((graph.zoom * 100).toLocaleString(Qt.locale(translator.language), 'f', 1))
              + "  ·  " + qsTr("nodes %1").arg(graph.nodeCount.toLocaleString(Qt.locale(translator.language), 'f', 0))
              + "  ·  " + qsTr("links %1").arg(graph.edgeCount.toLocaleString(Qt.locale(translator.language), 'f', 0))
    }
}
