import QtQuick
import QtQuick.Controls

// Rotary parameter knob with label + value readout. value in [from..to].
Item {
    id: root
    property real from: 0
    property real to: 1
    property real value: 0
    property string label: ""
    property string suffix: ""
    property int decimals: 0
    property color tint: Theme.accent
    signal moved(real v)
    implicitWidth: 76; implicitHeight: 92

    function norm() { return (value - from) / (to - from) }
    Column {
        anchors.fill: parent; spacing: 2
        Item {
            width: parent.width; height: width
            Canvas {
                id: c; anchors.fill: parent
                property real n: root.norm()
                onNChanged: requestPaint()
                onPaint: {
                    var ctx = getContext("2d"); ctx.reset();
                    var cx = width/2, cy = height/2, r = width/2 - 6;
                    var a0 = Math.PI*0.75, a1 = Math.PI*2.25;
                    ctx.lineWidth = 5; ctx.lineCap = "round";
                    ctx.strokeStyle = Theme.stroke;
                    ctx.beginPath(); ctx.arc(cx,cy,r,a0,a1); ctx.stroke();
                    ctx.strokeStyle = root.tint;
                    ctx.beginPath(); ctx.arc(cx,cy,r,a0,a0+(a1-a0)*n); ctx.stroke();
                }
                Component.onCompleted: requestPaint()
            }
            MouseArea {
                anchors.fill: parent
                property real startY; property real startVal
                onPressed: { startY = mouse.y; startVal = root.value }
                onPositionChanged: {
                    var dv = (startY - mouse.y) / 140 * (root.to - root.from);
                    var v = Math.max(root.from, Math.min(root.to, startVal + dv));
                    root.value = v; root.moved(v);
                }
            }
        }
        Label { width: parent.width; horizontalAlignment: Text.AlignHCenter
            text: root.value.toFixed(root.decimals) + root.suffix
            color: Theme.text; font.pixelSize: Theme.fontS }
        Label { width: parent.width; horizontalAlignment: Text.AlignHCenter
            text: root.label; color: Theme.textDim; font.pixelSize: Theme.fontS }
    }
}
