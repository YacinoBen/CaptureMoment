import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// This component assumes 'serializerController' and 'controller' are available
// in the QML context via QmlContextSetup.setContextProperty().
// Ex: serializerController = QmlContextSetup.getSerializerController()
//     controller = QmlContextSetup.getControllerMainScene()

ColumnLayout {
    id: root
    spacing: 10

    // --- Save Button ---
    Button {
        id: saveButton
        text: "Save Operations to XMP"
        enabled: controller.imageWidth > 0 // Enable only if an image is loaded

        onClicked: {
            console.log("Attempting to save operations...");
            // Retrieve the current image path from the controller
            // You might need to implement a method like getCurrentImagePath() in ImageControllerBase
            // and expose it via Q_PROPERTY or a Q_INVOKABLE method.
            // For this example, assume it exists.
            var imagePath = controller.currentImagePath; // <-- Method to implement in ImageControllerBase
            if (imagePath && imagePath.length > 0) {
                console.log("Saving operations for image:", imagePath);
                // Retrieve the list of active operations from the OperationStateManager
                // via the controller. You'll need to expose a method for this.
                // For example, controller.operationStateManager.getActiveOperations()
                // or controller.getActiveOperations() (method to implement).
                // For this example, assume controller.getActiveOperations() exists.
                var operations = controller.getActiveOperations(); // <-- Method to implement in ImageControllerBase
                if (operations) {
                    console.log("Calling serializerController.saveOperations with", operations.length, "operations");
                    serializerController.saveOperations(imagePath, operations);
                } else {
                    console.error("Could not get active operations from controller.");
                }
            } else {
                console.error("No valid image path available for saving.");
            }
        }
    }

    // --- Load Button ---
    Button {
        id: loadButton
        text: "Load Operations from XMP"
        enabled: controller.imageWidth > 0 // Enable only if an image is loaded

        onClicked: {
            console.log("Attempting to load operations...");
            var imagePath = controller.currentImagePath; // <-- Method to implement in ImageControllerBase
            if (imagePath && imagePath.length > 0) {
                console.log("Loading operations for image:", imagePath);
                serializerController.loadOperations(imagePath);
            } else {
                console.error("No valid image path available for loading.");
            }
        }
    }

    // --- Status Indicator ---
    Text {
        id: statusText
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        color: "green" // Default color for success messages
        text: "Ready" // Initial message

        // You can change the color for errors if needed
        // color: isLoading || isSaving ? "orange" : loadError ? "red" : saveError ? "red" : "green"
    }

    // --- Connections to SerializerController signals ---
    Connections {
        target: serializerController

        function onOperationsSaved() {
            console.log("Operations saved successfully!");
            statusText.text = "Operations saved!";
            statusText.color = "green"; // Success message
            // Optional: Reset the message after a delay
            // Timer { interval: 3000; running: true; onTriggered: statusText.text = "Ready" }
        }

        function onOperationsSaveFailed(errorMsg) {
            console.error("Failed to save operations:", errorMsg);
            statusText.text = "Save failed: " + errorMsg;
            statusText.color = "red"; // Error message
        }

        function onOperationsLoaded(loadedOperations) { // The signal transmits the loaded operations
            console.log("Operations loaded successfully! Count:", loadedOperations.length);
            statusText.text = "Operations loaded! Applying...";
            statusText.color = "green"; // Initial success message
            // Here, you should send the loaded operations to OperationStateManager
            // via the controller or directly to OperationStateManager if exposed.
            // For example, if controller has a method to apply a list of operations:
            // controller.applyOperations(loadedOperations); // <-- Method to implement
            // Or if OperationStateManager is exposed directly:
            // operationStateManager.applyOperations(loadedOperations); // <-- Method to implement
            // For this example, assume controller.applyOperations is implemented.
            if (loadedOperations && loadedOperations.length > 0) {
                 controller.applyOperations(loadedOperations); // <-- Method to implement in ImageControllerBase
                 console.log("Applied", loadedOperations.length, "loaded operations.");
                 statusText.text = "Operations loaded and applied!";
            } else {
                 console.info("No operations found in XMP file, but load was successful.");
                 statusText.text = "No operations found in XMP.";
            }
        }

        function onOperationsNotFoundOnLoad() {
            console.info("No operations found in XMP file for the current image.");
            statusText.text = "No operations found in XMP.";
            statusText.color = "orange"; // Info, not an error
        }

        function onOperationsLoadFailed(errorMsg) {
            console.error("Failed to load operations:", errorMsg);
            statusText.text = "Load failed: " + errorMsg;
            statusText.color = "red"; // Error message
        }
    }

    // --- Note on methods to implement in ImageControllerBase ---
    // For this QML to work, you must implement in ImageControllerBase (and potentially in OperationStateManager) :
    // 1. Q_PROPERTY(QString currentImagePath READ getCurrentImagePath NOTIFY imagePathChanged) // or a Q_INVOKABLE method
    // 2. Q_INVOKABLE std::vector<Core::Operations::OperationDescriptor> getActiveOperations(); // or similar
    // 3. Q_INVOKABLE void applyOperations(const std::vector<Core::Operations::OperationDescriptor>& operations);
    // And emit the appropriate signals (imagePathChanged) when the path changes.
}
