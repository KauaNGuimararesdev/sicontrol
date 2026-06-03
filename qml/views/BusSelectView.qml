import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Step 2: pick the mix bus to control. Access control on the console may
// restrict which buses a given HiQnet node may touch (shown as disabled).
Item {
    id: view
    property string title: "Bus Select"

    GridView {
        id: grid
        anchors.fill: parent; anchors.margins: Theme.pad
        cellWidth: width / 2; cellHeight: 88
        model: Mixer.profile ? 24 : 24      // mix bus count
        delegate: Item {
            width: grid.cellWidth; height: grid.cellHeight
            Rectangle {
                anchors.fill: parent; anchors.margins: 6; radius: Theme.radius
                color: Mixer.selectedBus === index ? Theme.panelHi : Theme.panel
                border.color: Mixer.selectedBus === index ? Theme.accent : Theme.stroke
                border.width: Mixer.selectedBus === index ? 2 : 1
                ColumnLayout {
                    anchors.centerIn: parent; spacing: 2
                    Rectangle { Layout.alignment: Qt.AlignHCenter
                        width: 30; height: 5; radius: 2
                        color: Theme.busColors[index % Theme.busColors.length] }
                    Label { text: "Mix " + (index + 1); color: Theme.text
                        font.pixelSize: Theme.fontM; font.bold: true
                        Layout.alignment: Qt.AlignHCenter }
                    Label { text: (index % 4 === 0) ? "Stereo" : "Mono"
                        color: Theme.textDim; font.pixelSize: Theme.fontS
                        Layout.alignment: Qt.AlignHCenter }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        Mixer.selectedBus = index;
                        StackView.view.push("MixView.qml")
                    }
                }
            }
        }
    }
}
