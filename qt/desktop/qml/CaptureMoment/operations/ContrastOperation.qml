import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

Item {
    id: contrastOperation

    implicitHeight: sliderControl.implicitHeight

    SliderControl {
        id: sliderControl
        anchors.fill: parent

        label: qsTr("Contrast")
        value: contrastControl.value
        from: contrastControl.minimum
        to: contrastControl.maximum
        stepSize: 0.01

        onValueChanged: {
            console.log("Value Contrast : ", value)
            contrastControl.setValue(value )
        }
    }
}
