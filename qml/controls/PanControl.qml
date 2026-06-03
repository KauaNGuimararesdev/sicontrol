import QtQuick
import QtQuick.Controls

// Horizontal pan control, value -1 (L) .. +1 (R).
Item {
    id: root
    property real value: 0
    signal moved(real v)
    implicitHeight: 38; implicitWidth: 200
    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width; height: 6; radius: 3; color: Theme.stroke
        Rectangle { anchors.horizontalCenter: parent.horizontalCenter
            width: 2; height: 12; y: -3; color: Theme.textDim }
    }
    Rectangle {
        id: knob; width: 22; height: 22; radius: 11
        color: Theme.panelHi; border.color: Theme.accent; border.width: 2
        anchors.verticalCenter: parent.verticalCenter
        x: (parent.width - width) * (root.value + 1) / 2
    }
    Label { text: "L"; color: Theme.textDim; font.pixelSize: Theme.fontS
        anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter }
    Label { text: "R"; color: Theme.textDim; font.pixelSize: Theme.fontS
        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter }
    MouseArea {
        anchors.fill: parent
        onPositionChanged: set(mouse.x); onPressed: set(mouse.x)
        function set(mx) {
            var t = Math.max(0, Math.min(1, mx / parent.width));
            root.value = t * 2 - 1; root.moved(root.value);
        }
    }
}
