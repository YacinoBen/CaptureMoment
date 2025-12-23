import QtQuick
import QtQuick.Controls

MenuBar {
    id: appMenuBar
    
    signal newFileTriggered()
    signal openFileTriggered()
    signal saveTriggered()
    signal exitTriggered()
    signal undoTriggered()
    signal redoTriggered()
    signal preferencesTriggered()
    signal aboutTriggered()
    
    Menu {
        title: qsTr("&File")
        
        Action {
            text: qsTr("&New")
            onTriggered: appMenuBar.newFileTriggered()
        }
        Action {
            text: qsTr("&Open")
            onTriggered: appMenuBar.openFileTriggered()
        }
        MenuSeparator { }
        Action {
            text: qsTr("&Save")
            onTriggered: appMenuBar.saveTriggered()
        }
        MenuSeparator { }
        Action {
            text: qsTr("&Exit")
            onTriggered: appMenuBar.exitTriggered()
        }
    }
    
    Menu {
        title: qsTr("&Edit")
        
        Action {
            text: qsTr("&Undo")
            onTriggered: appMenuBar.undoTriggered()
        }
        Action {
            text: qsTr("&Redo")
            onTriggered: appMenuBar.redoTriggered()
        }
        MenuSeparator { }
        Action {
            text: qsTr("&Preferences")
            onTriggered: appMenuBar.preferencesTriggered()
        }
    }
    
    Menu {
        title: qsTr("&Help")
        
        Action {
            text: qsTr("&About CaptureMoment")
            onTriggered: appMenuBar.aboutTriggered()
        }
    }
}
