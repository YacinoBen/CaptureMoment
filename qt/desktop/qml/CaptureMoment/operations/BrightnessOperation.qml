import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

Item {
    id: brightnessOperation

    implicitHeight: sliderControl.implicitHeight

    SliderControl {
        id: sliderControl
        anchors.fill: parent

        label: "Brightness"
        value: brightnessControl.value
        from: brightnessControl.minimum
        to: brightnessControl.maximum
        stepSize: 0.01

        onValueChanged: {
            console.log("Value Brightness : ", value)
            brightnessControl.setValue(value )
        }
    }
}
