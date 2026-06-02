import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Matrix routing: a grid of source -> matrix-output crosspoints. Tap a cell to
// toggle the send on/off; the knob row sets the level for the selected cell.
// LR/Mono master enables live along the bottom.
Item {
    id: view
    property string title: "Matrix"
    property int matrixOuts: 8
    property int sources: 16
    property int selSource: 0
    property int selOut: 0
    property real cellLevel: -10

    ColumnLayout {
        anchors.fill: parent; anchors.margins: Theme.pad; spacing: Theme.pad

        Flickable {
            Layout.fillWidth: true; Layout.fillHeight: true
            contentWidth: gridCol.width; contentHeight: gridCol.height; clip: true
            Column {
                id: gridCol; spacing: 2
                Repeater {
                    model: view.sources
                    Row {
                        property int srcIdx: index
                        spacing: 2
                        Rectangle { width: 60; height: 30; color: Theme.panel
                            border.color: Theme.stroke; radius: 4
                            Label { anchors.centerIn: parent; text: "Src " + (srcIdx+1)
                                color: Theme.textDim; font.pixelSize: Theme.fontS } }
                        Repeater {
                            model: view.matrixOuts
                            Rectangle {
                                width: 38; height: 30; radius: 4
                                property bool active: false
                                color: active ? Theme.accent : Theme.panelHi
                                border.color: (view.selSource===srcIdx && view.selOut===index)
                                              ? Theme.text : Theme.stroke
                                MouseArea { anchors.fill: parent
                                    onClicked: {
                                        view.selSource = srcIdx; view.selOut = index;
                                        parent.active = !parent.active;
                                        Mixer.setMatrixOn(index, srcIdx, parent.active);
                                    } }
                            }
                        }
                    }
                }
                Row {
                    spacing: 2
                    Rectangle { width: 60; height: 24; color: "transparent" }
                    Repeater { model: view.matrixOuts
                        Rectangle { width: 38; height: 24; color: "transparent"
                            Label { anchors.centerIn: parent; text: "M"+(index+1)
                                color: Theme.textDim; font.pixelSize: Theme.fontS } } }
                }
            }
        }

        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: Theme.panel; radius: Theme.radius; border.color: Theme.stroke }
            RowLayout {
                anchors.fill: parent; spacing: 12
                Label { text: "Src " + (view.selSource+1) + " \u2192 M" + (view.selOut+1)
                    color: Theme.text; font.bold: true }
                Slider {
                    Layout.fillWidth: true; from: -90; to: 10; value: view.cellLevel
                    onMoved: { view.cellLevel = value;
                        Mixer.setMatrix(view.selOut, view.selSource, value) }
                }
                Label { text: view.cellLevel.toFixed(1) + " dB"; color: Theme.textDim }
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Button { text: "LR On"; Layout.fillWidth: true; checkable: true; checked: true
                onToggled: Mixer.setMasterOn(0, checked)
                background: Rectangle { radius:6; color: parent.checked?Theme.ok:Theme.stroke }
                contentItem: Text { text:parent.text; color:"#0c0f12"; font.bold:true
                    horizontalAlignment:Text.AlignHCenter; verticalAlignment:Text.AlignVCenter } }
            Button { text: "Mono On"; Layout.fillWidth: true; checkable: true; checked: true
                onToggled: Mixer.setMasterOn(1, checked)
                background: Rectangle { radius:6; color: parent.checked?Theme.ok:Theme.stroke }
                contentItem: Text { text:parent.text; color:"#0c0f12"; font.bold:true
                    horizontalAlignment:Text.AlignHCenter; verticalAlignment:Text.AlignVCenter } }
        }
    }
}
