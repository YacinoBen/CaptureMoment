import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: leftPanel
    
    color: Material.backgroundColor
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12
        
        // Header
        Text {
            text: "Tools & History"
            color: Material.foreground
            font.bold: true
            font.pixelSize: 14
        }
        
        // Future content placeholder
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 4
            
            Text {
                anchors.centerIn: parent
                text: "Future: Tools & History"
                color: "black"
                font.pixelSize: 12
            }
        }
    }
}
