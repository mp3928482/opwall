import QtQuick

// One numbered panel: a badge, a title and a subtitle, a tag on the right,
// and corner brackets in the panel's color. Content goes in `body`.
Item {
    id: panel

    property int number: 1
    property string title
    property string subtitle
    property string tag
    property color tint: theme.accent
    default property alias body: content.data

    Rectangle {
        anchors.fill: parent
        color: theme.surface
        radius: theme.radius
        border.color: Qt.rgba(1, 1, 1, 0.07)
        border.width: 1
    }

    // Corner brackets, so the panels read as tracked frames on a wall.
    Repeater {
        model: 4
        delegate: Item {
            required property int index
            readonly property bool atRight: index % 2 === 1
            readonly property bool atBottom: index >= 2
            width: 14
            height: 14
            x: atRight ? panel.width - width + 3 : -3
            y: atBottom ? panel.height - height + 3 : -3
            opacity: 0.85
            Rectangle { width: parent.width; height: 2; color: panel.tint; y: atBottom ? parent.height - 2 : 0 }
            Rectangle { width: 2; height: parent.height; color: panel.tint; x: atRight ? parent.width - 2 : 0 }
        }
    }

    Row {
        id: header
        anchors { left: parent.left; right: tagText.left; top: parent.top; leftMargin: 10; topMargin: 9; rightMargin: 8 }
        spacing: 8
        clip: true

        Rectangle {
            width: badge.implicitWidth + 10
            height: 16
            radius: Math.min(4, theme.radius)
            color: panel.tint
            Text {
                id: badge
                anchors.centerIn: parent
                text: (panel.number < 10 ? "0" : "") + panel.number
                color: "#101012"
                font.pixelSize: 10
                font.bold: true
            }
        }
        Text {
            text: panel.title.toUpperCase()
            color: theme.foreground
            font.pixelSize: 11
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: "·  " + panel.subtitle
            color: theme.muted
            font.pixelSize: 11
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    Text {
        id: tagText
        anchors { right: parent.right; rightMargin: 12; verticalCenter: header.verticalCenter }
        text: panel.tag
        color: panel.tint
        font.pixelSize: 10
    }

    Item {
        id: content
        anchors { fill: parent; topMargin: 34; leftMargin: 10; rightMargin: 10; bottomMargin: 10 }
        clip: true
    }
}
