import QtQuick
import QtQuick.Controls
import "../js/EqMath.js" as Eq

// Draws the combined EQ response from a list of band descriptors and lets the
// user drag band handles. bands is an array of:
//   { enabled, kind, fc, gain, q, slope, color }
Item {
    id: root
    property var bands: []
    property real fMin: 20
    property real fMax: 20000
    property real gMax: 18           // +/- dB shown
    signal bandChanged(int index, real fc, real gain)

    implicitHeight: 220

    Rectangle { anchors.fill: parent; color: Theme.panel
        border.color: Theme.stroke; radius: Theme.radius }

    Canvas {
        id: grid; anchors.fill: parent; anchors.margins: 1
        onPaint: {
            var ctx = getContext("2d"); ctx.reset();
            ctx.strokeStyle = Theme.stroke; ctx.lineWidth = 1;
            var decades = [20,50,100,200,500,1000,2000,5000,10000,20000];
            for (var i=0;i<decades.length;++i){
                var x = Eq.xForFreq(decades[i], width, root.fMin, root.fMax);
                ctx.beginPath(); ctx.moveTo(x,0); ctx.lineTo(x,height); ctx.stroke();
            }
            // 0 dB line
            ctx.strokeStyle = Theme.textDim;
            ctx.beginPath(); ctx.moveTo(0,height/2); ctx.lineTo(width,height/2); ctx.stroke();
        }
    }

    Canvas {
        id: curve; anchors.fill: parent; anchors.margins: 1
        property var b: root.bands
        onBChanged: requestPaint()
        onPaint: {
            var ctx = getContext("2d"); ctx.reset();
            var w = Math.floor(width);
            var data = Eq.curve(root.bands, w, root.fMin, root.fMax);
            ctx.strokeStyle = Theme.accent; ctx.lineWidth = 2; ctx.beginPath();
            for (var x=0;x<w;++x){
                var y = height/2 - (data[x]/root.gMax) * (height/2);
                if (x===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
            }
            ctx.stroke();
        }
        Component.onCompleted: requestPaint()
    }

    // draggable band handles
    Repeater {
        model: root.bands
        Rectangle {
            visible: modelData.enabled && (modelData.kind === 'bell'
                     || modelData.kind === 'lowshelf' || modelData.kind === 'highshelf')
            width: 16; height: 16; radius: 8
            color: modelData.color || Theme.accent
            border.color: "#0c0f12"; border.width: 2
            x: Eq.xForFreq(modelData.fc, root.width, root.fMin, root.fMax) - 8
            y: root.height/2 - (modelData.gain/root.gMax)*(root.height/2) - 8
            MouseArea {
                anchors.fill: parent
                onPositionChanged: {
                    var px = mapToItem(root, mouse.x, mouse.y);
                    var fc = Eq.logFreqAt(Math.max(0,Math.min(root.width,px.x)),
                                          root.width, root.fMin, root.fMax);
                    var g  = (root.height/2 - px.y)/(root.height/2)*root.gMax;
                    g = Math.max(-root.gMax, Math.min(root.gMax, g));
                    root.bandChanged(index, fc, g);
                }
            }
        }
    }
}
