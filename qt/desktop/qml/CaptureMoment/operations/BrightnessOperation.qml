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
        value: brightnessControl.value * 100
        from: brightnessControl.minimum * 100
        to: brightnessControl.maximum * 100
        stepSize: 1

        onValueChanged: {
            console.log("Value Brightness : ", value / 100)
            brightnessControl.setValue(value / 100)
        }
    }
}
