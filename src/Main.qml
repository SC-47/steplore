import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 1100
    height: 720
    minimumWidth: 920
    minimumHeight: 560
    visible: true
    title: "Steplore"
    color: theme.background

    Material.theme: theme.light ? Material.Light : Material.Dark
    Material.accent: theme.accent
    Material.background: theme.background
    Material.foreground: theme.foreground

    readonly property bool rightToLeft: Qt.application.layoutDirection === Qt.RightToLeft

    // In the order of Backend::Algorithm.
    readonly property var names: [
        qsTr("Bubble sort"), qsTr("Selection sort"), qsTr("Insertion sort"),
        qsTr("Merge sort"), qsTr("Quick sort"), qsTr("Binary search")
    ]
    readonly property var ideas: [
        qsTr("Swap neighbours that are out of order, pass after pass, until the biggest values settle at the end."),
        qsTr("Find the largest value that is left and put it at the end."),
        qsTr("Take the next value and slide it left into the sorted part."),
        qsTr("Split the list in half, sort each half, then merge the two sorted halves."),
        qsTr("Pick a pivot, move smaller values to its left, then sort each side."),
        qsTr("Look at the middle of a sorted list and throw away the half that cannot hold the target.")
    ]
    // In the order of Backend::Language. Names of languages stay as they are.
    readonly property var languages: ["C#", "Python", "Rust", "TypeScript", "JavaScript"]
    // Each algorithm's cost as the list grows, for the code as it is shown:
    // the bubble sort here never stops early, so even its best case is n².
    readonly property var complexities: [
        { best: "O(n²)", average: "O(n²)", worst: "O(n²)", space: "O(1)" },
        { best: "O(n²)", average: "O(n²)", worst: "O(n²)", space: "O(1)" },
        { best: "O(n)", average: "O(n²)", worst: "O(n²)", space: "O(1)" },
        { best: "O(n log n)", average: "O(n log n)", worst: "O(n log n)", space: "O(n)" },
        { best: "O(n log n)", average: "O(n log n)", worst: "O(n²)", space: "O(log n)" },
        { best: "O(1)", average: "O(log n)", worst: "O(log n)", space: "O(1)" }
    ]

    function fade(color, alpha) {
        return Qt.rgba(color.r, color.g, color.b, alpha)
    }

    // In the order of Backend::Mark.
    function markColor(mark) {
        switch (mark) {
        case 1: return theme.accent
        case 2: return theme.colors.red
        case 3: return theme.colors.green
        case 4: return theme.colors.magenta
        case 5: return fade(theme.foreground, 0.14)
        default: return fade(theme.foreground, 0.5)
        }
    }

    // What the current step did, in the order of Backend::Note.
    function noteText() {
        const a = backend.noteA, b = backend.noteB
        switch (backend.note) {
        case 0:
            return backend.target >= 0
                ? qsTr("Find %1 in the sorted list. Press Space to play, or → for one step.").arg(backend.target)
                : qsTr("Press Space to play, or → for one step.")
        case 1: return qsTr("Compare %1 and %2.").arg(a).arg(b)
        case 2: return qsTr("Swap %1 and %2.").arg(a).arg(b)
        case 3: return qsTr("Sorted.")
        case 4: return qsTr("%1 is the largest so far.").arg(a)
        case 5: return qsTr("%1 is already in its place.").arg(a)
        case 6: return qsTr("Take %1 as the key.").arg(a)
        case 7: return qsTr("Move %1 one place to the right.").arg(a)
        case 8: return qsTr("Put the key %1 into the gap.").arg(a)
        case 9: return qsTr("Work on positions %1 to %2.").arg(a).arg(b)
        case 10: return qsTr("Write %1 back at position %2.").arg(a).arg(b)
        case 11: return qsTr("Use %1 as the pivot.").arg(a)
        case 12: return qsTr("Put the pivot %1 in its final place.").arg(a)
        case 13: return qsTr("The middle value is %1. Is it %2?").arg(a).arg(b)
        case 14: return qsTr("%1 is too small, so search the right half.").arg(a)
        case 15: return qsTr("%1 is too big, so search the left half.").arg(a)
        case 16: return qsTr("Found %1 at position %2.").arg(a).arg(b)
        case 17: return qsTr("%1 is not in the list.").arg(a)
        }
        return ""
    }

    Shortcut { sequence: "?"; onActivated: help.opened ? help.close() : help.open() }
    Shortcut { sequence: "Q"; onActivated: Qt.quit() }
    Shortcut { sequence: "Space"; onActivated: backend.toggle() }
    Shortcut { sequences: ["Right", "L"]; onActivated: backend.next() }
    Shortcut { sequences: ["Left", "H"]; onActivated: backend.previous() }
    Shortcut { sequence: "Home"; onActivated: backend.seek(0) }
    Shortcut { sequence: "End"; onActivated: backend.seek(backend.stepCount - 1) }
    Shortcut { sequence: "R"; onActivated: backend.shuffle() }
    Shortcut { sequences: ["+", "="]; onActivated: backend.faster() }
    Shortcut { sequence: "-"; onActivated: backend.slower() }
    Shortcut { sequence: "T"; onActivated: backend.cycleLanguage(1) }
    Shortcut { sequence: "Shift+T"; onActivated: backend.cycleLanguage(-1) }
    Shortcut { sequence: "1"; onActivated: backend.algorithm = 0 }
    Shortcut { sequence: "2"; onActivated: backend.algorithm = 1 }
    Shortcut { sequence: "3"; onActivated: backend.algorithm = 2 }
    Shortcut { sequence: "4"; onActivated: backend.algorithm = 3 }
    Shortcut { sequence: "5"; onActivated: backend.algorithm = 4 }
    Shortcut { sequence: "6"; onActivated: backend.algorithm = 5 }

    // A button drawn from the theme alone, so its caption reads on any accent.
    component Pill: Rectangle {
        id: pill
        property string text
        property bool active: false
        signal clicked()

        implicitWidth: caption.implicitWidth + 28
        implicitHeight: 36
        radius: Math.min(theme.radius, height / 2)
        color: active ? theme.accent
             : hover.hovered ? Qt.rgba(theme.foreground.r, theme.foreground.g, theme.foreground.b, 0.08)
             : "transparent"
        border.color: active ? theme.accent : theme.colors.selection

        Label {
            id: caption
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            text: pill.text
            color: pill.active ? theme.accentForeground : theme.foreground
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        HoverHandler { id: hover }
        TapHandler { onTapped: pill.clicked() }
    }

    // Right-to-left languages mirror the window; the Translator sets the
    // application's layout direction.
    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: win.rightToLeft
        LayoutMirroring.childrenInherit: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: win.names
                    delegate: Pill {
                        required property int index
                        required property string modelData
                        objectName: "tab" + index
                        Layout.fillWidth: true
                        // The search is not one of the sorts, so it stands apart.
                        Layout.leftMargin: index === 5 ? 20 : 0
                        text: (index + 1) + "  " + modelData
                        active: backend.algorithm === index
                        onClicked: backend.algorithm = index
                    }
                }
            }

            Label {
                objectName: "idea"
                Layout.fillWidth: true
                text: win.ideas[backend.algorithm]
                wrapMode: Text.WordWrap
                opacity: 0.8
            }

            Flow {
                Layout.fillWidth: true
                spacing: 28

                Repeater {
                    model: [
                        { name: qsTr("Best time"), value: win.complexities[backend.algorithm].best },
                        { name: qsTr("Average time"), value: win.complexities[backend.algorithm].average },
                        { name: qsTr("Worst time"), value: win.complexities[backend.algorithm].worst },
                        { name: qsTr("Extra space"), value: win.complexities[backend.algorithm].space }
                    ]
                    delegate: Row {
                        required property int index
                        required property var modelData
                        spacing: 8

                        Label {
                            text: modelData.name
                            opacity: 0.7
                        }
                        Label {
                            objectName: "complexity" + index
                            text: modelData.value
                            color: theme.accent
                            font.bold: true
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 20

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10

                    // Position 0 stays on the left in every language, as the
                    // code beside it counts, so the bars are placed by x.
                    Item {
                        id: bars
                        objectName: "bars"
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        readonly property real slot: width / Math.max(1, backend.size)
                        readonly property real floor: height - 26
                        readonly property real tallest: Math.max(0, floor - 28)

                        Repeater {
                            model: backend.size
                            delegate: Item {
                                id: column
                                required property int index
                                readonly property int value: backend.values[index] ?? 0
                                readonly property int mark: backend.marks[index] ?? 0

                                x: index * bars.slot
                                width: bars.slot
                                height: bars.height

                                Rectangle {
                                    objectName: "bar" + column.index
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    y: bars.floor - height
                                    width: parent.width * 0.74
                                    height: bars.tallest * column.value / backend.maxValue
                                    radius: Math.min(4, theme.radius)
                                    color: win.markColor(column.mark)
                                    Behavior on height { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
                                    Behavior on color { ColorAnimation { duration: 90 } }
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottom: parent.bottom
                                    text: column.value
                                    font.pixelSize: 13
                                    opacity: column.mark === 5 ? 0.35 : 0.9
                                }
                            }
                        }

                        // The height the search is looking for.
                        Rectangle {
                            objectName: "targetLine"
                            visible: backend.target >= 0
                            x: 0
                            y: bars.floor - bars.tallest * backend.target / backend.maxValue
                            width: parent.width
                            height: 1
                            color: theme.accent

                            Label {
                                anchors.bottom: parent.top
                                anchors.bottomMargin: 2
                                text: qsTr("Target: %1").arg(backend.target)
                                color: theme.accent
                                font.bold: true
                            }
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 18

                        Repeater {
                            model: [
                                { mark: 1, name: qsTr("Comparing") },
                                { mark: 2, name: qsTr("Writing") },
                                { mark: 4, name: qsTr("Pivot or key") },
                                { mark: 3, name: qsTr("Done") }
                            ]
                            delegate: Row {
                                required property var modelData
                                spacing: 6

                                Rectangle {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 12
                                    height: 12
                                    radius: Math.min(4, theme.radius)
                                    color: win.markColor(modelData.mark)
                                }
                                Label {
                                    text: modelData.name
                                    font.pixelSize: 13
                                    opacity: 0.8
                                }
                            }
                        }
                    }
                }

                // Code reads left to right in every language.
                ColumnLayout {
                    Layout.preferredWidth: 540
                    Layout.maximumWidth: 540
                    Layout.fillHeight: true
                    spacing: 8
                    LayoutMirroring.enabled: false
                    LayoutMirroring.childrenInherit: true

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Repeater {
                            model: win.languages
                            delegate: Pill {
                                required property int index
                                required property string modelData
                                objectName: "language" + index
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                text: modelData
                                active: backend.language === index
                                onClicked: backend.language = index
                            }
                        }
                    }

                    Rectangle {
                        id: codePanel
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: win.fade(theme.foreground, 0.05)
                        border.color: theme.colors.selection
                        radius: theme.radius
                        clip: true

                        // A long listing, or one with long lines, shrinks to
                        // fit rather than being cut off.
                        readonly property int longest: backend.code.reduce((most, line) => Math.max(most, line.length), 1)
                        readonly property real rowHeight: Math.max(10, Math.min(26, (height - 28) / Math.max(1, backend.code.length)))
                        readonly property int codeSize: Math.max(8, Math.floor(Math.min(14, rowHeight * 0.72, (width - 76) / (longest * 0.61))))

                        Column {
                            x: 12
                            y: 14
                            width: parent.width - 24

                            Repeater {
                                model: backend.code
                                delegate: Rectangle {
                                    required property int index
                                    required property string modelData
                                    readonly property bool current: backend.line === index
                                    objectName: "line" + index
                                    width: parent.width
                                    height: codePanel.rowHeight
                                    radius: Math.min(4, theme.radius)
                                    color: current ? win.fade(theme.accent, 0.28) : "transparent"

                                    Label {
                                        x: 6
                                        width: 22
                                        anchors.verticalCenter: parent.verticalCenter
                                        horizontalAlignment: Text.AlignRight
                                        text: index + 1
                                        font.family: "monospace"
                                        font.pixelSize: Math.max(8, codePanel.codeSize - 1)
                                        opacity: 0.4
                                    }
                                    Label {
                                        objectName: "code" + index
                                        x: 40
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData
                                        textFormat: Text.PlainText
                                        font.family: "monospace"
                                        font.pixelSize: codePanel.codeSize
                                        // A learner types >= and ==, so show the
                                        // characters, not the font's ligatures.
                                        font.features: { "liga": 0, "calt": 0 }
                                        font.bold: parent.current
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Label {
                objectName: "note"
                Layout.fillWidth: true
                text: win.noteText()
                font.pixelSize: 20
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Pill {
                    objectName: "previous"
                    text: "←"
                    onClicked: backend.previous()
                }
                Pill {
                    objectName: "play"
                    Layout.minimumWidth: 140
                    // Each button names its key; a key is spelled the same in every language.
                    text: (backend.playing ? qsTr("Pause") : qsTr("Play")) + " (Space)"
                    active: true
                    onClicked: backend.toggle()
                }
                Pill {
                    objectName: "next"
                    text: "→"
                    onClicked: backend.next()
                }
                Pill {
                    objectName: "shuffle"
                    text: qsTr("Shuffle") + " (R)"
                    onClicked: backend.shuffle()
                }

                Slider {
                    objectName: "progress"
                    Layout.fillWidth: true
                    // The arrow keys step the algorithm, so the slider never takes focus.
                    focusPolicy: Qt.NoFocus
                    from: 0
                    to: Math.max(1, backend.stepCount - 1)
                    stepSize: 1
                    value: backend.step
                    onMoved: backend.seek(Math.round(value))
                }

                Label {
                    objectName: "stepLabel"
                    text: qsTr("Step %1 of %2").arg(backend.step + 1).arg(backend.stepCount)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 24

                Label {
                    objectName: "comparisons"
                    text: qsTr("Comparisons: %1").arg(backend.comparisons)
                    opacity: 0.8
                }
                Label {
                    objectName: "writes"
                    text: qsTr("Writes: %1").arg(backend.writes)
                    opacity: 0.8
                }
                Item { Layout.fillWidth: true }
                // The speed has five steps; a button fades at the end it can't pass.
                Pill {
                    objectName: "slower"
                    implicitWidth: 36
                    text: "-"
                    opacity: backend.speed > 1 ? 1 : 0.35
                    onClicked: backend.slower()
                }
                Label {
                    objectName: "speed"
                    text: qsTr("Speed: %1").arg(backend.speed)
                    opacity: 0.8
                }
                Pill {
                    objectName: "faster"
                    implicitWidth: 36
                    text: "+"
                    opacity: backend.speed < 5 ? 1 : 0.35
                    onClicked: backend.faster()
                }
                Item { implicitWidth: 8 }
                // The way in to the shortcuts for whoever doesn't know the key yet.
                Pill {
                    objectName: "helpButton"
                    implicitWidth: 36
                    text: "?"
                    onClicked: help.open()
                }
            }
        }
    }

    Popup {
        id: help
        objectName: "help"
        anchors.centerIn: Overlay.overlay
        modal: true
        focus: true
        padding: 24
        background: Rectangle {
            color: theme.background
            border.color: theme.colors.selection
            radius: theme.radius
        }

        ColumnLayout {
            spacing: 8
            // A popup is not an Item and doesn't inherit the window's
            // mirroring, so its content opts in.
            LayoutMirroring.enabled: win.rightToLeft
            LayoutMirroring.childrenInherit: true

            Repeater {
                model: [
                    { keys: "Space", action: qsTr("Play or pause") },
                    { keys: "→  L", action: qsTr("One step forward") },
                    { keys: "←  H", action: qsTr("One step back") },
                    { keys: "Home  End", action: qsTr("Jump to the start or the end") },
                    { keys: "1 – 6", action: qsTr("Choose an algorithm") },
                    { keys: "T  Shift+T", action: qsTr("Next or previous programming language") },
                    { keys: "R", action: qsTr("Shuffle new numbers") },
                    { keys: "+  -", action: qsTr("Faster or slower") },
                    { keys: "?", action: qsTr("Show or hide the shortcuts") },
                    { keys: "Q", action: qsTr("Quit") }
                ]
                delegate: RowLayout {
                    required property var modelData
                    spacing: 24

                    Label {
                        Layout.preferredWidth: 96
                        text: modelData.keys
                        color: theme.accent
                        font.bold: true
                    }
                    Label { text: modelData.action }
                }
            }
        }
    }
}
