import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import CaptureMoment.UI.Rendering.Painted 1.0
import CaptureMoment.UI.Rendering.SGS 1.0
import CaptureMoment.UI.Rendering.RHI 1.0

Rectangle {
    id: displayArea

    color: "#1a1a1a"

    signal openImageClicked()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            id: imageContainer
            Layout.fillWidth: true
            Layout.fillHeight: true

            QMLSGSImageItem {
                id: imageDisplay
                anchors.fill: parent

                onImageSizeChanged: {
                    console.log("DisplayArea.qml::onHeightChanged: width: ", imageDisplay.imageWidth)
                    console.log("DisplayArea.qml::onWidthChanged: height: ", imageDisplay.imageHeight)
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20
                visible: controller.imageWidth === 0
                z: 1

                Text {
                    text: "📷"
                    font.pixelSize: 64
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "No Image Loaded"
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "Click 'Open Image' to start editing"
                    color: "#AAAAAA"
                    font.pixelSize: 14
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        Rectangle {
            id: infoFooter
            color: "#252525"
            Layout.fillWidth: true
            Layout.preferredHeight: infoText.implicitHeight + 20
            Layout.alignment: Qt.AlignBottom
            visible: infoText.visible

            Text {
                id: infoText
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 10

                text: "Source: " + controller.imageWidth + "x" + controller.imageHeight +
                      " | Display: " + (controller.displayManager ? controller.displayManager.displayImageSize.width : 0) +
                      "x" + (controller.displayManager ? controller.displayManager.displayImageSize.height : 0)

                color: "#CCCCCC"
                font.pixelSize: 12
                visible: controller.imageWidth > 0
            }
        }
    }

    Component.onCompleted: {
       // controller.setPaintedImageItemFromQml(imageDisplay)
        controller.setSGSImageItemFromQml(imageDisplay)
       // controller.setRHIImageItemFromQml(imageDisplay)
    }

    function updateViewport() {
        if (!controller || !controller.displayManager) {
            console.log("!controller || !controller.displayManager")
            return;
        }

        var availableHeight = height - (infoFooter.visible ? infoFooter.height : 0);

        controller.displayManager.setViewportSize(Qt.size(width, availableHeight));
    }

    onWidthChanged: {
        updateViewport();
    }

    onHeightChanged: {
        updateViewport();
    }

    FileDialog {
        id: fileDialog
        title: "Open Image"

        onAccepted: {
            console.log("Loading image:", selectedFile)
            controller.loadImageFromUrl(selectedFile)
        }
    }

    Connections {
        target: controller

        function onImageLoaded(loadedWidth, loadedHeight) {
            console.log("DisplayArea::onImageLoaded, Image loaded:", loadedWidth, "x", loadedHeight)

            if (controller.displayManager) {
                updateViewport();
                controller.displayManager.fitToView();
            }
        }

        function onImageLoadFailed(error) {
            console.error("DisplayArea::onImageLoadedFailed, Load failed:", error)
        }
    }

    function openFile() {
        fileDialog.open()
    }
}
