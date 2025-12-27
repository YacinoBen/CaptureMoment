import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

import CaptureMoment.desktop

ApplicationWindow {
    id: window
    visible: true
    width: 1400
    height: 900
    title: "CaptureMoment - Photo Editor"

    // Application MenuBar (File, Edit, Help)
    menuBar: AppMenuBar {
        id: appMenuBar

        onNewFileTriggered: console.log("New file")
        onOpenFileTriggered: console.log("Open file")
        onSaveTriggered: console.log("Save file")
        onExitTriggered: window.close()
        onUndoTriggered: console.log("Undo")
        onRedoTriggered: console.log("Redo")
        onPreferencesTriggered: console.log("Preferences")
        onAboutTriggered: console.log("About")
    }

    // Use EditorView which contains the full layout
    EditorView {
        anchors.fill: parent
    }

    // Setup performed when the component is fully loaded
    Component.onCompleted: {
        console.log("Application started")
    }
}
