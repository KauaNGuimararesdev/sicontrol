import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Mute groups: eight master mutes the engineer can fire. Also exposes output
// delay (taps) for the selected bus.
Item {
    id: view
    property string title: "Mute Groups"

    ColumnLayout {
        anchors.fill: parent; anchors.margins: Theme.pad; spacing: Theme.pad
        GridLayout {
            Layout.fillWidth: true; columns: 4; rowSpacing: 8; columnSpacing: 8
            Repeater {
                model: 8
                Button {
                    Layout.fillWidth: true; Layout.preferredHeight: 70
                    checkable: true
                    text: "MUTE\n" + (index+1)
                    onToggled: Mixer.setMuteGroup(index, checked)
                    background: Rectangle { radius: Theme.radius
                        color: parent.checked ? Theme.danger : Theme.panelHi
                        border.color: Theme.stroke }
                    contentItem: Text { text: parent.text; color: Theme.text; font.bold: true
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                }
            }
        }

        Label { text: "Output Delay (Tap) \u2013 Mix " + (Mixer.selectedBus+1)
            color: Theme.text; font.bold: true; Layout.topMargin: Theme.pad }
        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: Theme.panel; radius: Theme.radius; border.color: Theme.stroke }
            RowLayout {
                anchors.fill: parent; spacing: 12
                property bool dly: false; property real ms: 0
                Switch { text: "On"; onToggled: { parent.dly = checked
                        Mixer.setOutputDelay(Mixer.selectedBus, parent.ms, checked) }
                    contentItem: Text { text: parent.text; color: Theme.text
                        leftPadding: parent.indicator.width + 6; verticalAlignment: Text.AlignVCenter } }
                Slider { Layout.fillWidth: true; from: 0; to: 500; value: parent.ms
                    onMoved: { parent.ms = value
                        Mixer.setOutputDelay(Mixer.selectedBus, value, parent.dly) } }
                Label { text: parent.ms.toFixed(0) + " ms"; color: Theme.textDim }
            }
        }
        Item { Layout.fillHeight: true }
    }
}
