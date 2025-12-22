import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

Rectangle {
    id: centerPanel

    color: "#2a2a2a"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toolbar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: "#1E1E1E"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                Button {
                    text: "Open Image"
                    onClicked: displayArea.openFile()
                }

                Item {
                    Layout.fillWidth: true
                }

                Text {
                    text: controller.imageWidth > 0 ? "Loaded" : "No image"
                    color: controller.imageWidth > 0 ? "#4CAF50" : "#999999"
                    font.pixelSize: 12
                }
            }
        }

        // Display Area
        DisplayArea {
            id: displayArea
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
