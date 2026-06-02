import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SiControl
import "../controls"

// The contribution-levels page: a horizontally scrolling row of channel faders
// for the selected mix bus, with pan when the bus is stereo. Below sits the
// view-group filter and quick links to the engineer screens.
Item {
    id: view
    property string title: "Mix " + (Mixer.selectedBus + 1)

    ColumnLayout {
        anchors.fill: parent; spacing: 0

        // --- fader strip ---
        Flickable {
            Layout.fillWidth: true; Layout.fillHeight: true
            contentWidth: row.width; clip: true; flickableDirection: Flickable.HorizontalFlick
            Row {
                id: row; height: parent.height; spacing: 2; padding: 8
                Repeater {
                    model: Mixer.channels
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        visible: modelData.inViewGroup
                        width: Theme.faderW + 10; height: row.height - 16
                        color: index % 2 ? Theme.panel : Theme.bg
                        radius: 6; border.color: Theme.stroke

                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 4; spacing: 4

                            // ON / mute
                            Button {
                                Layout.fillWidth: true; Layout.preferredHeight: 26
                                text: modelData.mute ? "MUTE" : "ON"
                                background: Rectangle { radius: 4
                                    color: modelData.mute ? Theme.danger
                                          : (modelData.on ? Theme.accentDim : Theme.stroke) }
                                contentItem: Text { text: parent.text; color: Theme.text
                                    font.pixelSize: Theme.fontS; font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter }
                                onClicked: modelData.mute = !modelData.mute
                            }

                            ViSiFader {
                                Layout.fillWidth: true; Layout.fillHeight: true
                                value: modelData.fader
                                meter: modelData.meter
                                label: modelData.name
                                tint: Theme.busColors[Mixer.selectedBus % Theme.busColors.length]
                                onMoved: function(db) { modelData.fader = db }
                            }

                            // pan (stereo buses)
                            PanControl {
                                Layout.fillWidth: true; visible: Mixer.busStereo
                                value: modelData.pan
                                onMoved: function(v) { modelData.pan = v }
                            }

                            // open per-channel engineer strip
                            Button {
                                Layout.fillWidth: true; Layout.preferredHeight: 24
                                text: "EDIT"
                                background: Rectangle { radius: 4; color: Theme.panelHi
                                    border.color: Theme.stroke }
                                contentItem: Text { text: parent.text; color: Theme.textDim
                                    font.pixelSize: Theme.fontS
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter }
                                onClicked: StackView.view.push("ChannelStripView.qml",
                                    { channelIndex: index })
                            }
                        }
                    }
                }
            }
        }

        // --- bottom bar: view groups + engineer screens ---
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 64
            color: Theme.panel; border.color: Theme.stroke
            RowLayout {
                anchors.fill: parent; anchors.margins: 8; spacing: 8
                Repeater {
                    model: [
                        { t: "Matrix",   q: "MatrixView.qml" },
                        { t: "Mutes",    q: "MuteGroupsView.qml" },
                        { t: "View Grp", q: "" },
                        { t: "Settings", q: "SettingsView.qml" }
                    ]
                    Button {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        text: modelData.t
                        background: Rectangle { radius: 6; color: Theme.panelHi
                            border.color: Theme.stroke }
                        contentItem: Text { text: parent.text; color: Theme.text
                            font.pixelSize: Theme.fontS; horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter }
                        onClicked: if (modelData.q !== "") StackView.view.push(modelData.q)
                    }
                }
            }
        }
    }
}
