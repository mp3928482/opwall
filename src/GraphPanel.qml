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
        surface: theme.surface
        fontFamily: Qt.application.font.family
        running: !backend.paused && root.visible
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
