import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Control {
    id: sliderControl

    property string label: "Control"
    property real value: 0
    property real from: 0
    property real to: 100
    property real stepSize: 1

    implicitHeight: layout.implicitHeight + 16

    leftPadding: 8
    rightPadding: 8

    contentItem: RowLayout {
        id: layout
        spacing: 8

        Text {
            text: sliderControl.label
            color: Material.foreground
            font.pixelSize: 13
            Layout.preferredWidth: 70
        }

        Slider {
            id: slider
            Layout.fillWidth: true
            value: sliderControl.value
            from: sliderControl.from
            to: sliderControl.to
            stepSize: sliderControl.stepSize

            onMoved: {
                sliderControl.value = value
            }
            
            background: Rectangle {
                x: slider.leftPadding
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                implicitWidth: 200
                implicitHeight: 4
                width: slider.availableWidth
                height: implicitHeight
                radius: 2
                color: Material.midlight  // Grey background
                
                // Progress bar
                Rectangle {
                    height: parent.height
                    radius: 2
                    color: Material.accent
                    
                    // Progress from center (0) to left or right
                    x: {
                        const midPoint = parent.width / 2
                        const range = sliderControl.to - sliderControl.from
                        const normalizedValue = (sliderControl.value - sliderControl.from) / range
                        const progress = normalizedValue * parent.width
                        
                        if (sliderControl.value >= 0) {
                            // Value is positive: progress from middle to right
                            return midPoint
                        } else {
                            // Value is negative: progress from middle to left
                            return progress
                        }
                    }
                    
                    width: {
                        const midPoint = parent.width / 2
                        const range = sliderControl.to - sliderControl.from
                        const normalizedValue = (sliderControl.value - sliderControl.from) / range
                        const progress = normalizedValue * parent.width
                        
                        if (sliderControl.value >= 0) {
                            // Positive: width from middle to current value
                            return progress - midPoint
                        } else {
                            // Negative: width from current value to middle
                            return midPoint - progress
                        }
                    }
                }
            }
            
            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * slider.availableWidth - width / 2
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                implicitWidth: 18
                implicitHeight: 18
                radius: 9
                color: slider.pressed ? Material.accent : Material.primary
            }
        }

        SpinBox {
            id: spinBox
            value: Math.round(sliderControl.value)
            from: Math.round(sliderControl.from)
            to: Math.round(sliderControl.to)
            stepSize: Math.round(sliderControl.stepSize)
            editable: true
            Layout.preferredWidth: 105
            Layout.preferredHeight: 35

            onValueChanged: {
                sliderControl.value = value
            }
        }
    }
}
