import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: topPanel

    color: "#1E1E1E"
    height: 56

    signal fileClicked()
    signal bibliothequeClicked()
    signal collectionsClicked()
    signal parametresClicked()
    signal btnLoadImageClicked()

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 24

        // Logo
        Text {
            text: "CaptureMoment"
            color: "white"
            font.bold: true
            font.pixelSize: 16
        }

        // Navigation buttons

        Text {
            text: "Library"
            color: "#AAAAAA"
            font.pixelSize: 13

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: topPanel.bibliothequeClicked()
            }
        }

        Text {
            text: "Collections"
            color: "#AAAAAA"
            font.pixelSize: 13

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: topPanel.collectionsClicked()
            }
        }

        Text {
            text: "Parametres"
            color: "#AAAAAA"
            font.pixelSize: 13

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: topPanel.parametresClicked()
            }
        }


        Button {
            text: "Open Image"
            onClicked: topPanel.btnLoadImageClicked()
        }

        Item {
            Layout.fillWidth: true
        }
    }
}
