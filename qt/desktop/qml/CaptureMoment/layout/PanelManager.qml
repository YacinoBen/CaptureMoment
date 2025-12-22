import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

ColumnLayout {
    id: editorLayout
    spacing: 0

    // Top Panel (full width)
    TopPanel {
        id: topPanel
        Layout.fillWidth: true

        onFileClicked: {
            console.log("File view clicked")
            // Afficher la vue File dans le center panel
        }

        onBibliothequeClicked: {
            console.log("Bibliotheque view clicked")
            // Afficher la vue Bibliotheque dans le center panel
        }

        onCollectionsClicked: {
            console.log("Collections view clicked")
            // Afficher la vue Collections dans le center panel
        }

        onParametresClicked: {
            console.log("Parametres view clicked")
            // Afficher la vue Parametres dans le center panel
        }
    }

    // Main content with SplitView (Left/Center/Right)
    SplitView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        orientation: Qt.Horizontal

        // Left Panel
        LeftPanel {
            id: leftPanel
            SplitView.preferredWidth: 250
            SplitView.minimumWidth: 150
            SplitView.maximumWidth: 400
        }

        // Center Panel (Image Display)
        CenterPanel {
            id: centerPanel
            SplitView.fillWidth: true

            SplitView.minimumWidth: 800
        }

        // Right Panel (Operations)
        RightPanel {
            id: rightPanel
            SplitView.preferredWidth: 350
            SplitView.minimumWidth: 300
            SplitView.maximumWidth: 500
        }
    }

    // Bottom Panel (full width)
    BottomPanel {
        id: bottomPanel
        Layout.fillWidth: true
        Layout.preferredHeight: 120
    }
}
