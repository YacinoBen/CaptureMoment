import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

Item {
    id: blacksOperation

    implicitHeight: sliderControl.implicitHeight

    SliderControl {
        id: sliderControl
        anchors.fill: parent

        label: qsTr("Blacks")
        value: blacksControl.value
        from: blacksControl.minimum
        to: blacksControl.maximum
        stepSize: 0.01

        onValueChanged: {
            console.log("Value Blacks : ", value)
            blacksControl.setValue(value )
        }
    }
}
