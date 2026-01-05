import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

Item {
    id: highlightsOperation

    implicitHeight: sliderControl.implicitHeight

    SliderControl {
        id: sliderControl
        anchors.fill: parent

        label: qsTr("Highlights")
        value: highlightsControl.value
        from: highlightsControl.minimum
        to: highlightsControl.maximum
        stepSize: 0.01

        onValueChanged: {
            console.log("Value Highlights : ", value)
            highlightsControl.setValue(value )
        }
    }
}
