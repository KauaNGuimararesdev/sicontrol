import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls"

// Per-channel engineer processing: head-amp (gain/48V/phase), 4-band
// parametric EQ + HPF with a live response curve, compressor and gate.
// These are the engineer functions the original ViSi Listen lacked.
Item {
    id: view
    property int channelIndex: 0
    property var ch: Mixer.channel(channelIndex)
    property string title: ch ? ch.name + " \u2013 Processing" : "Channel"

    // local EQ band state -> drives the graph and pushes to console
    property var eqBands: [
        { enabled: true, kind: 'hp',        fc: 80,    gain: 0,  q: 0.7, slope: 24, color: Theme.textDim },
        { enabled: true, kind: 'lowshelf',  fc: 120,   gain: 0,  q: 0.7, color: Theme.busColors[1] },
        { enabled: true, kind: 'bell',      fc: 500,   gain: 0,  q: 1.4, color: Theme.busColors[0] },
        { enabled: true, kind: 'bell',      fc: 2500,  gain: 0,  q: 1.4, color: Theme.busColors[2] },
        { enabled: true, kind: 'highshelf', fc: 8000,  gain: 0,  q: 0.7, color: Theme.busColors[3] }
    ]
    function pushEq() {
        // bands 1..4 are the parametric bands (band 0 is HPF)
        for (var i = 1; i < eqBands.length; ++i)
            Mixer.setEq(channelIndex, i - 1, eqBands[i].fc, eqBands[i].gain, eqBands[i].q);
    }

    Flickable {
        anchors.fill: parent; contentHeight: col.height; clip: true
        ColumnLayout {
            id: col; width: parent.width; spacing: Theme.pad
            anchors.margins: Theme.pad
            anchors.left: parent.left; anchors.right: parent.right
            anchors.leftMargin: Theme.pad; anchors.rightMargin: Theme.pad
            anchors.topMargin: Theme.pad

            // --- head amp ---
            Frame {
                Layout.fillWidth: true; Layout.topMargin: Theme.pad
                background: Rectangle { color: Theme.panel; radius: Theme.radius; border.color: Theme.stroke }
                RowLayout {
                    anchors.fill: parent; spacing: 16
                    RotaryKnob {
                        label: "Gain"; from: -6; to: 60; decimals: 0; suffix: "dB"
                        value: ch ? ch.gain : 0
                        onMoved: function(v){ if (ch) ch.gain = v }
                    }
                    ColumnLayout {
                        spacing: 8
                        Switch { text: "48V"; checked: ch ? ch.phantom : false
                            onToggled: if (ch) ch.phantom = checked
                            contentItem: Text { text: parent.text; color: Theme.text
                                leftPadding: parent.indicator.width + 6
                                verticalAlignment: Text.AlignVCenter } }
                        Switch { text: "\u00f8 Phase"; checked: ch ? ch.phase : false
                            onToggled: if (ch) ch.phase = checked
                            contentItem: Text { text: parent.text; color: Theme.text
                                leftPadding: parent.indicator.width + 6
                                verticalAlignment: Text.AlignVCenter } }
                    }
                    Item { Layout.fillWidth: true }
                }
            }

            // --- EQ ---
            Label { text: "Equaliser"; color: Theme.text; font.bold: true; font.pixelSize: Theme.fontM }
            EqGraph {
                id: eqGraph; Layout.fillWidth: true; bands: view.eqBands
                onBandChanged: function(i, fc, gain) {
                    var b = view.eqBands.slice();
                    b[i].fc = Math.round(fc); b[i].gain = Math.round(gain*10)/10;
                    view.eqBands = b; view.pushEq();
                }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 8
                Repeater {
                    model: 4   // four parametric bands
                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { text: "B" + (index+1); color: Theme.textDim
                            font.pixelSize: Theme.fontS; Layout.alignment: Qt.AlignHCenter }
                        RotaryKnob {
                            Layout.alignment: Qt.AlignHCenter
                            label: "Freq"; from: 20; to: 20000; suffix: ""
                            value: view.eqBands[index+1].fc
                            onMoved: function(v){ var b=view.eqBands.slice();
                                b[index+1].fc=Math.round(v); view.eqBands=b; view.pushEq() }
                        }
                        RotaryKnob {
                            Layout.alignment: Qt.AlignHCenter
                            label: "Q"; from: 0.3; to: 8; decimals: 1
                            value: view.eqBands[index+1].q
                            onMoved: function(v){ var b=view.eqBands.slice();
                                b[index+1].q=v; view.eqBands=b; view.pushEq() }
                        }
                    }
                }
            }

            // --- compressor ---
            Label { text: "Compressor"; color: Theme.text; font.bold: true; font.pixelSize: Theme.fontM }
            Frame {
                Layout.fillWidth: true
                background: Rectangle { color: Theme.panel; radius: Theme.radius; border.color: Theme.stroke }
                RowLayout {
                    anchors.fill: parent; spacing: 10
                    property var c: ({ thr:-20, ratio:3, att:10, rel:120, makeup:0 })
                    RotaryKnob { label:"Thr"; from:-60; to:0; suffix:"dB"; value:parent.c.thr
                        onMoved: function(v){ parent.c.thr=v; push() } }
                    RotaryKnob { label:"Ratio"; from:1; to:20; decimals:1; value:parent.c.ratio
                        onMoved: function(v){ parent.c.ratio=v; push() } }
                    RotaryKnob { label:"Att"; from:0.5; to:100; suffix:"ms"; value:parent.c.att
                        onMoved: function(v){ parent.c.att=v; push() } }
                    RotaryKnob { label:"Rel"; from:20; to:1000; suffix:"ms"; value:parent.c.rel
                        onMoved: function(v){ parent.c.rel=v; push() } }
                    RotaryKnob { label:"Gain"; from:0; to:24; suffix:"dB"; value:parent.c.makeup
                        onMoved: function(v){ parent.c.makeup=v; push() } }
                    function push(){ Mixer.setCompressor(view.channelIndex, c.thr, c.ratio, c.att, c.rel, c.makeup) }
                }
            }

            // --- gate ---
            Label { text: "Gate"; color: Theme.text; font.bold: true; font.pixelSize: Theme.fontM }
            Frame {
                Layout.fillWidth: true; Layout.bottomMargin: Theme.pad
                background: Rectangle { color: Theme.panel; radius: Theme.radius; border.color: Theme.stroke }
                RowLayout {
                    anchors.fill: parent; spacing: 10
                    property var g: ({ thr:-40, range:-30, att:1, hold:50, rel:200 })
                    RotaryKnob { label:"Thr"; from:-80; to:0; suffix:"dB"; value:parent.g.thr
                        onMoved: function(v){ parent.g.thr=v; push() } }
                    RotaryKnob { label:"Range"; from:-60; to:0; suffix:"dB"; value:parent.g.range
                        onMoved: function(v){ parent.g.range=v; push() } }
                    RotaryKnob { label:"Att"; from:0.1; to:50; decimals:1; suffix:"ms"; value:parent.g.att
                        onMoved: function(v){ parent.g.att=v; push() } }
                    RotaryKnob { label:"Hold"; from:0; to:500; suffix:"ms"; value:parent.g.hold
                        onMoved: function(v){ parent.g.hold=v; push() } }
                    RotaryKnob { label:"Rel"; from:20; to:2000; suffix:"ms"; value:parent.g.rel
                        onMoved: function(v){ parent.g.rel=v; push() } }
                    function push(){ Mixer.setGate(view.channelIndex, g.thr, g.range, g.att, g.hold, g.rel) }
                }
            }
        }
    }
}
