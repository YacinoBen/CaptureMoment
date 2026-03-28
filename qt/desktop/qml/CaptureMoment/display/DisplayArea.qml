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

    // Local properties to force info display immediately upon loading
    property int currentSourceWidth: 0
    property int currentSourceHeight: 0
    property var displaySize: controller.displayManager ? controller.displayManager.displayImageSize : Qt.size(0, 0)

    signal openImageClicked()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            id: imageContainer
            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true

            QMLSGSImageItem {
                id: imageDisplay

                // Center the item in the container instead of filling it
                anchors.centerIn: parent

                // Set size to the specific display size calculated by the backend
                width: (controller && controller.displayManager) ? controller.displayManager.displayImageSize.width : 0
                height: (controller && controller.displayManager) ? controller.displayManager.displayImageSize.height : 0

                onImageSizeChanged: {
                    console.log("DisplayArea.qml::onImageSizeChanged: width:", imageDisplay.imageWidth,
                    "height:", imageDisplay.imageHeight)
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20
                visible: currentSourceWidth === 0
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
            Layout.minimumHeight: 30

            // Visibility driven by local properties updated on load
            visible: currentSourceWidth > 0

            Text {
                id: infoText
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 10

                text:"Source: " + currentSourceWidth + "x" + currentSourceHeight +
                " | Display: " + displaySize.width + "x" + displaySize.height

                color: "#CCCCCC"
                font.pixelSize: 12
                visible: true
            }
        }
    }

    Component.onCompleted: {
       // controller.setPaintedImageItemFromQml(imageDisplay)
        controller.setSGSImageItemFromQml(imageDisplay)
       //controller.setRHIImageItemFromQml(imageDisplay)
    }

    function updateViewport() {
        if (!controller || !controller.displayManager) {
            return;
        }

        // Safety check: ensure dimensions are valid
        if (width <= 0 || height <= 0) return;

        // Calculate footer height.
        // We check if visible AND height > 0 to avoid layout glitches during initialization.
        var footerHeight = (infoFooter.visible && infoFooter.height > 0) ? infoFooter.height : 0;
        var availableHeight = height - footerHeight;

        controller.displayManager.setViewportSize(Qt.size(width, availableHeight));
    }

    onWidthChanged: updateViewport()
    onHeightChanged: updateViewport()

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

            // Update local properties immediately to trigger UI changes (footer visibility)
            currentSourceWidth = loadedWidth;
            currentSourceHeight = loadedHeight;

            if (controller.displayManager) {
                // This ensures the Layout engine has finished resizing the footer
                // before we calculate the available height for the image.
                // This prevents the image from overflowing on first load.
                Qt.callLater(updateViewport);
                Qt.callLater(controller.displayManager.fitToView);
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
