pragma Singleton
import QtQuick

// Central design tokens. A dark, console-style palette in the spirit of the
// Soundcraft Si surface, defined from scratch (no proprietary assets).
QtObject {
    // surfaces
    readonly property color bg:        "#0e1114"
    readonly property color panel:     "#171c21"
    readonly property color panelHi:   "#1f262d"
    readonly property color stroke:    "#2b333b"

    // text
    readonly property color text:      "#e9edf0"
    readonly property color textDim:   "#9aa6b0"

    // accents (Si blue family seen in the original UI)
    readonly property color accent:    "#3b9ddb"
    readonly property color accentDim: "#27628b"
    readonly property color ok:        "#46c46e"
    readonly property color warn:      "#e8b341"
    readonly property color danger:    "#e1574c"

    // bus / function colour coding
    readonly property var busColors: [
        "#3b9ddb", "#46c46e", "#e8b341", "#e1574c",
        "#9b6cff", "#23c4c4", "#ff8f4c", "#c45fa0"
    ]

    // meter gradient stops
    readonly property color meterLow:  "#46c46e"
    readonly property color meterMid:  "#e8b341"
    readonly property color meterHigh: "#e1574c"

    // metrics
    readonly property int   radius:    8
    readonly property int   pad:       12
    readonly property int   faderW:    54
    readonly property real  fontL:     20
    readonly property real  fontM:     15
    readonly property real  fontS:     12
}
