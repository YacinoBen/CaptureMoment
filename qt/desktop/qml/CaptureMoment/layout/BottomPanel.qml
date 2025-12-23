import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: bottomPanel
    
    color: Material.Cyan
    border.color: Material.Cyan

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        // Header
        Text {
            text: "Collections"
            color: "white"
            font.bold: true
            font.pixelSize: 14
        }
        
        // Future content placeholder
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Material.Teal
            radius: 4
            
            Text {
                anchors.centerIn: parent
                text: "Future: Collections & Gallery"
                color: Material.foreground
                font.pixelSize: 12
            }
        }
    }
}
