import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Step 1 of the ViSi flow: set this device's HiQnet identity and connect to a
// console by IP (plus a discovery list placeholder). Mirrors the original
// "Device Selection" screen.
Item {
    id: view
    property string title: "Devices"
    signal proceed()

    ColumnLayout {
        anchors.fill: parent; anchors.margins: Theme.pad; spacing: Theme.pad

        Label {
            text: "Connect to your Soundcraft Si console. Ensure the console and this device share the same Wi-Fi network."
            color: Theme.textDim; font.pixelSize: Theme.fontM; wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // --- this device identity ---
        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: Theme.panel; radius: Theme.radius; border.color: Theme.stroke }
            ColumnLayout {
                anchors.fill: parent; spacing: 8
                Label { text: "This device (HiQnet)"; color: Theme.text; font.bold: true }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Label { text: "Node"; color: Theme.textDim; Layout.preferredWidth: 70 }
                    TextField { id: selfNode; text: "1001"; Layout.fillWidth: true
                        color: Theme.text; inputMethodHints: Qt.ImhDigitsOnly }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Label { text: "My IP"; color: Theme.textDim; Layout.preferredWidth: 70 }
                    TextField { id: selfIp; text: "192.168.1.50"; Layout.fillWidth: true; color: Theme.text }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Label { text: "Mask"; color: Theme.textDim; Layout.preferredWidth: 70 }
                    TextField { id: selfMask; text: "255.255.255.0"; Layout.fillWidth: true; color: Theme.text }
                }
            }
        }

        // --- console target ---
        Frame {
            Layout.fillWidth: true
            background: Rectangle { color: Theme.panel; radius: Theme.radius; border.color: Theme.stroke }
            ColumnLayout {
                anchors.fill: parent; spacing: 8
                Label { text: "Console"; color: Theme.text; font.bold: true }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Label { text: "IP"; color: Theme.textDim; Layout.preferredWidth: 70 }
                    TextField { id: deviceIp; text: "192.168.1.10"; Layout.fillWidth: true; color: Theme.text }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Label { text: "Node"; color: Theme.textDim; Layout.preferredWidth: 70 }
                    TextField { id: deviceNode; text: "1"; Layout.fillWidth: true
                        color: Theme.text; inputMethodHints: Qt.ImhDigitsOnly }
                }
            }
        }

        Button {
            Layout.fillWidth: true; Layout.preferredHeight: 52
            text: Mixer.connectionState === "Online" ? "Connected \u2014 Continue"
                  : Mixer.connectionState === "Disconnected" ? "Connect" : Mixer.connectionState + "\u2026"
            background: Rectangle { radius: Theme.radius
                color: Mixer.connectionState === "Online" ? Theme.ok : Theme.accent }
            contentItem: Text { text: parent.text; color: "#0c0f12"; font.bold: true
                font.pixelSize: Theme.fontM
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            onClicked: {
                if (Mixer.connectionState === "Online") { view.proceed(); return }
                Mixer.configureSelf(parseInt(selfNode.text), selfIp.text, selfMask.text);
                Mixer.connectConsole(deviceIp.text, parseInt(deviceNode.text));
            }
        }

        Item { Layout.fillHeight: true }
    }
}
