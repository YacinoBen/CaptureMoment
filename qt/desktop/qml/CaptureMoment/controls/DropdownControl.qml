import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

Item {
    id: dropdownControl
    
    property string label: "Dropdown"
    property var model: []
    property int currentIndex: 0
    property var currentValue: model.length > 0 ? model[currentIndex] : ""
    
    signal indexChanged(int index)
    signal valueChanged(var value)
    
    implicitHeight: layout.implicitHeight + Theme.spacing * 2
    
    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: Theme.spacing
        
        Text {
            text: dropdownControl.label
            color: Theme.textColor
            font.pixelSize: Theme.fontSizeBody
            Layout.preferredWidth: 80
        }
        
        ComboBox {
            id: comboBox
            model: dropdownControl.model
            currentIndex: dropdownControl.currentIndex
            Layout.fillWidth: true
            
            onCurrentIndexChanged: {
                dropdownControl.currentIndex = currentIndex
                dropdownControl.currentValue = model[currentIndex]
                dropdownControl.indexChanged(currentIndex)
                dropdownControl.valueChanged(currentValue)
            }
        }
    }
}
