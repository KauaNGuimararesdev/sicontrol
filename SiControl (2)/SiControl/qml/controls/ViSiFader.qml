import QtQuick
import QtQuick.Controls

// Vertical fader. Reports value in dB (-90..+10). Mirrors the ViSi fader feel:
// large touch target, integrated bar-meter on the left, value readout on top.
Item {
    id: root
    property real minDb: -90
    property real maxDb: 10
    property real value: -90        // dB
    property real meter: 0          // 0..1 input level for the inline meter
    property string label: ""
    property color tint: Theme.accent
    signal moved(real db)

    implicitWidth: Theme.faderW
    implicitHeight: 320

    function dbToNorm(db) {
        // piecewise feel: more travel near unity, compressed at the bottom
        var t = (db - minDb) / (maxDb - minDb);
        return Math.max(0, Math.min(1, t));
    }
    function normToDb(t) { return minDb + t * (maxDb - minDb); }

    Column {
        anchors.fill: parent; spacing: 4
        Label {
            width: parent.width; horizontalAlignment: Text.AlignHCenter
            text: root.value <= root.minDb + 0.5 ? "-\u221e"
                  : (root.value > 0 ? "+" : "") + root.value.toFixed(1)
            color: Theme.text; font.pixelSize: Theme.fontS
        }

        Item {
            id: track
            width: parent.width
            height: parent.height - 48
            property real usable: height - cap.height

            // inline meter
            Rectangle {
                x: 6; width: 6; radius: 3
                anchors.bottom: parent.bottom
                height: parent.height * root.meter
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Theme.meterHigh }
                    GradientStop { position: 0.4; color: Theme.meterMid }
                    GradientStop { position: 1.0; color: Theme.meterLow }
                }
            }
            // groove
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 6; radius: 3; height: parent.height; color: Theme.stroke
            }
            // unity tick
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 22; height: 2; color: Theme.textDim
                y: track.usable * (1 - root.dbToNorm(0)) + cap.height/2
            }
            // cap
            Rectangle {
                id: cap
                width: parent.width - 6; height: 30; radius: 5
                x: 3
                y: track.usable * (1 - root.dbToNorm(root.value))
                color: Theme.panelHi; border.color: root.tint; border.width: 2
                Rectangle { anchors.centerIn: parent; width: parent.width-10; height: 3
                    radius: 2; color: root.tint }
            }
            MouseArea {
                anchors.fill: parent
                onPositionChanged: grab(mouse.y)
                onPressed: grab(mouse.y)
                function grab(my) {
                    var t = 1 - Math.max(0, Math.min(1, (my - cap.height/2) / track.usable));
                    var db = root.normToDb(t);
                    root.value = db; root.moved(db);
                }
            }
        }

        Label {
            width: parent.width; horizontalAlignment: Text.AlignHCenter
            text: root.label; color: Theme.textDim; font.pixelSize: Theme.fontS
            elide: Text.ElideRight
        }
    }
}
