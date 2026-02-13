import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

ColumnLayout {
    id: editorLayout
    spacing: 0

    TopPanel {
        id: topPanel
        Layout.fillWidth: true

        onFileClicked: {
            console.log("File view clicked")
        }

        onBibliothequeClicked: {
            console.log("Bibliotheque view clicked")
        }

        onCollectionsClicked: {
            console.log("Collections view clicked")
        }

        onParametresClicked: {
            console.log("Parametres view clicked")
        }

        onBtnLoadImageClicked: {
            console.log("button load image clicked")
            centerPanel.idDisplayArea.openFile()
        }
    }

    SplitView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        orientation: Qt.Horizontal

        LeftPanel {
            id: leftPanel
            SplitView.preferredWidth: 250
            SplitView.minimumWidth: 150
            SplitView.maximumWidth: 400
        }

        CenterPanel {
            id: centerPanel
            SplitView.fillWidth: true

            SplitView.minimumWidth: 800
        }

        RightPanel {
            id: rightPanel
            SplitView.preferredWidth: 350
            SplitView.minimumWidth: 300
            SplitView.maximumWidth: 500
        }
    }

    BottomPanel {
        id: bottomPanel
        Layout.fillWidth: true
        Layout.preferredHeight: 120
    }
}
