import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

Item {
    id: shadowsOperation

    implicitHeight: sliderControl.implicitHeight

    SliderControl {
        id: sliderControl
        anchors.fill: parent

        label: qsTr("Shadows")
        value: shadowsControl.value
        from: shadowsControl.minimum
        to: shadowsControl.maximum
        stepSize: 0.01

        onValueChanged: {
            console.log("Value Shadows : ", value)
            shadowsControl.setValue(value )
        }
    }
}
