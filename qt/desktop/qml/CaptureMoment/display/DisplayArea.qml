import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import CaptureMoment.UI.Painted.Rendering 1.0
import CaptureMoment.desktop

Rectangle {
    id: displayArea
    
    color: "#1a1a1a"
    
    signal openImageClicked()

    QMLPaintedImageItem {
        id: imageDisplay
        anchors.fill: parent

        onImageDimensionsChanged: {
            console.log("DisplayArea.qml::onImageDimensionsChanged: width: ", imageDisplay.imageWidth)
            console.log("DisplayArea.qml::onWidthChanged: height: ", imageDisplay.imageHeight)
        }
    }

    // Setup image display when loaded
    Component.onCompleted: {
        controller.setPaintedImageItemFromQml(imageDisplay)
        if (controller.displayManager) {
            controller.displayManager.setViewportSize(Qt.size(width, height))
        }
    }

    // Update viewport on resize
    onWidthChanged: {
        console.log("DisplayArea.qml::onWidthChanged: ", imageDisplay.imageWidth)
        if (controller.displayManager) {
            controller.displayManager.setViewportSize(Qt.size(width, height))
        }
    }

    onHeightChanged: {
        console.log("DisplayArea.qml::onWidthChanged: ", imageDisplay.imageHeight)

        if (controller.displayManager) {
            controller.displayManager.setViewportSize(Qt.size(width, height))
        }
    }

    // Empty state
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20
        visible: controller.imageWidth === 0

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

    // Info overlay (bottom-left)
    Text {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 20
        text: "Source: " + controller.imageWidth + "x" + controller.imageHeight +
              " | Display: " + (controller.displayManager ? controller.displayManager.displayImageSize.width : 0) +
              "x" + (controller.displayManager ? controller.displayManager.displayImageSize.height : 0)
        color: "#CCCCCC"
        font.pixelSize: 12
        visible: controller.imageWidth > 0
    }

    // File Dialog
    FileDialog {
        id: fileDialog
        title: "Open Image"

        onAccepted: {
            var path = selectedFile.toString()
            path = path.replace(/^(file:\/{3})|(qrc:\/{2})|(http:\/{2})/, "")
            var cleanPath = decodeURIComponent(path)
            console.log("Loading image:", cleanPath)
            controller.loadImage(cleanPath)
        }
    }


    // Monitor signals from the controller
    Connections {
        target: controller

        function onImageLoaded(width, height) {
            console.log("DisplayArea::onImageLoaded, Image loaded:", width, "x", height)

            if (controller.displayManager) {
                controller.displayManager.setViewportSize(Qt.size(window.width * 0.65, window.height * 0.85))
                controller.displayManager.fitToView()
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
