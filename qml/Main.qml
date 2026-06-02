import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "views"

ApplicationWindow {
    id: win
    visible: true
    width: 420; height: 840
    title: "SiControl"
    color: Theme.bg

    // ViSi flow: Device Select -> Bus Select -> Mix.  Engineer screens
    // (Channel strip, Matrix, Mute groups) open as pushes from Mix.
    header: ToolBar {
        height: 56
        background: Rectangle { color: Theme.panel; border.color: Theme.stroke }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 8
            ToolButton {
                text: "\u2039"; font.pixelSize: 26; visible: stack.depth > 1
                onClicked: stack.pop()
                contentItem: Text { text: parent.text; color: Theme.text; font: parent.font
                    verticalAlignment: Text.AlignVCenter }
                background: Item {}
            }
            Label {
                Layout.fillWidth: true
                text: stack.currentItem ? (stack.currentItem.title || "SiControl") : "SiControl"
                color: Theme.text; font.pixelSize: Theme.fontL; font.bold: true
            }
            // connection status pill
            Rectangle {
                radius: 11; height: 22; implicitWidth: stLbl.width + 22
                color: Mixer.connectionState === "Online" ? Theme.ok
                     : Mixer.connectionState === "Disconnected" ? Theme.danger : Theme.warn
                Label { id: stLbl; anchors.centerIn: parent; text: Mixer.connectionState
                    color: "#0c0f12"; font.pixelSize: Theme.fontS; font.bold: true }
            }
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: DeviceSelectView {
            onProceed: stack.push("views/BusSelectView.qml")
        }
    }

    // global toast for log/errors
    Connections {
        target: Mixer
        function onLogMessage(m) { toast.show(m) }
    }
    Rectangle {
        id: toast; anchors.bottom: parent.bottom; anchors.bottomMargin: 24
        anchors.horizontalCenter: parent.horizontalCenter
        color: Theme.panelHi; border.color: Theme.stroke; radius: 8
        opacity: 0; width: tl.width + 24; height: 36
        Label { id: tl; anchors.centerIn: parent; color: Theme.textDim; font.pixelSize: Theme.fontS }
        function show(t) { tl.text = t; opacity = 1; hide.restart() }
        Behavior on opacity { NumberAnimation { duration: 200 } }
        Timer { id: hide; interval: 2600; onTriggered: toast.opacity = 0 }
    }
}
