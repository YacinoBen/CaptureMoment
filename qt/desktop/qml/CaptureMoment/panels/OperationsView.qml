import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import CaptureMoment.desktop

ScrollView {
    id: operationsView

    contentWidth: availableWidth
    ScrollBar.vertical.policy: ScrollBar.AsNeeded

    ColumnLayout {
        width: operationsView.availableWidth
        spacing: 8

        TonePanel {
            id: tonePanel
            Layout.fillWidth: true
        }

        // Future: Color Panel
        // ColorPanel {
        //     id: colorPanel
        //     Layout.fillWidth: true
        // }

        // Future: Detail Panel
        // DetailPanel {
        //     id: detailPanel
        //     Layout.fillWidth: true
        // }

        // Future: Curves Panel
        // CurvesPanel {
        //     id: curvesPanel
        //     Layout.fillWidth: true
        // }
    }
}
