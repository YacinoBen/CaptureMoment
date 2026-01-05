import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

Item {
    id: whitesOperation

    implicitHeight: sliderControl.implicitHeight

    SliderControl {
        id: sliderControl
        anchors.fill: parent

        label: qsTr("Whites")
        value: whitesControl.value
        from: whitesControl.minimum
        to: whitesControl.maximum
        stepSize: 0.01

        onValueChanged: {
            console.log("Value Whites : ", value)
            whitesControl.setValue(value )
        }
    }
}
