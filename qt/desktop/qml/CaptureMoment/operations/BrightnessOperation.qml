import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

Item {
    id: brightnessOperation

    property real value: 0
    property real minBrightness: -100
    property real maxBrightness: 100

    implicitHeight: sliderControl.implicitHeight

    SliderControl {
        id: sliderControl
        anchors.fill: parent

        label: "Brightness"
        value: brightnessOperation.value
        from: brightnessOperation.minBrightness
        to: brightnessOperation.maxBrightness
        stepSize: 1

        onValueChanged: {
          //  brightnessOperation.value = sliderControl.value
        }
    }
}
