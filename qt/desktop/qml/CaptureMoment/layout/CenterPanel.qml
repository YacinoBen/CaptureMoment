import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

Rectangle {
    id: centerPanel

    property alias idDisplayArea: displayArea
    color: "#2a2a2a"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: "#1E1E1E"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 40
                anchors.verticalCenter: parent.verticalCenter

                text: controller.imageWidth > 0 ? "● "+qsTr("Loaded") : "○ "+ qsTr("No image")
                color: controller.imageWidth > 0 ? "#4CAF50" : "#888888"
                font.pixelSize: 11
                font.bold: true
            }
        }

        DisplayArea {
            id: displayArea
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
