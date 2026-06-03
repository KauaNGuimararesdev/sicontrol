import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Network + HiQnet settings and a live message log (handy while filling the
// parameter map: you can watch the raw rx values from the console).
Item {
    id: view
    property string title: "Settings"
    ColumnLayout {
        anchors.fill: parent; anchors.margins: Theme.pad; spacing: Theme.pad
        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: Theme.panel; radius: Theme.radius; border.color: Theme.stroke }
            ColumnLayout { anchors.fill: parent; spacing: 6
                Label { text: "Connection: " + Mixer.connectionState; color: Theme.text }
                Label { text: "Selected bus: Mix " + (Mixer.selectedBus+1); color: Theme.textDim }
                Button { text: "Disconnect"; onClicked: Mixer.disconnectConsole()
                    background: Rectangle { radius:6; color: Theme.stroke }
                    contentItem: Text { text:parent.text; color:Theme.text
                        horizontalAlignment:Text.AlignHCenter } }
            }
        }
        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: Theme.panel; radius: Theme.radius; border.color: Theme.stroke }
            ColumnLayout { anchors.fill: parent; spacing: 6
                Label { text: "Map a control (Learn)"; color: Theme.text; font.bold: true }
                Label {
                    text: "Pick what you're mapping, tap Arm, then move that\nexact control on the console. The app binds it."
                    color: Theme.textDim; font.pixelSize: Theme.fontS; wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    ComboBox {
                        id: kindBox; Layout.fillWidth: true
                        model: ["Fader","Pan","Mute","OnOff","Gain","EqFreq","EqGain","EqQ",
                                "HpFreq","CompThreshold","CompRatio","GateThreshold",
                                "MatrixGain","Delay"]
                    }
                    SpinBox { id: chBox; from: 1; to: 64; value: 1 }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Button {
                        Layout.fillWidth: true
                        text: Mixer.learnArmed ? "Arming - move it now (cancel)" : ("Arm: " + kindBox.currentText + " ch " + chBox.value)
                        onClicked: Mixer.learnArmed ? Mixer.cancelLearn()
                                                    : Mixer.armLearn(kindBox.currentText, chBox.value - 1, -1, -1)
                        background: Rectangle { radius:6; color: Mixer.learnArmed ? Theme.accent : Theme.stroke }
                        contentItem: Text { text:parent.text; color:Theme.text
                            horizontalAlignment:Text.AlignHCenter }
                    }
                }
            }
        }
        Label { text: "HiQnet log"; color: Theme.text; font.bold: true }
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true
            color: Theme.panel; radius: Theme.radius; border.color: Theme.stroke
            ListView {
                id: logView; anchors.fill: parent; anchors.margins: 8; clip: true
                model: ListModel { id: logModel }
                delegate: Label { text: msg; color: Theme.textDim; font.pixelSize: Theme.fontS
                    font.family: "monospace" }
            }
        }
        Connections {
            target: Mixer
            function onLogMessage(m) {
                logModel.append({ msg: m });
                if (logModel.count > 200) logModel.remove(0);
                logView.positionViewAtEnd();
            }
        }
    }
}
