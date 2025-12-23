import QtQuick
import QtQuick.Controls
import CaptureMoment.desktop

Rectangle {
    id: rightPanel
    
    color: "#1a1a1a"
    
    OperationsView {
        id: operationsView
        anchors.fill: parent
    }
}
