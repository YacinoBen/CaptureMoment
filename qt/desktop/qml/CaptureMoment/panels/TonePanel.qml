import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

CollapsiblePanel {
    id: tonePanel

    title: "Tone"


    content: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        BrightnessOperation {
            id: brightnessOp
            Layout.fillWidth: true
        }

        ContrastOperation {
            id: contrastOp
            Layout.fillWidth: true
        }
    }
}
