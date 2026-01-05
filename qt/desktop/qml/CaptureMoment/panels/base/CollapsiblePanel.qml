import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import CaptureMoment.desktop

Rectangle {
    id: collapsiblePanel

    property string title: "Panel"
    property bool isCollapsed: false
    default property alias content: contentContainer.data

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent


        PanelHeader {
            id: header
            title: collapsiblePanel.title
            isCollapsed: collapsiblePanel.isCollapsed
            Layout.fillWidth: true
            Layout.preferredHeight: 35

            onToggleCollapse: {
                collapsiblePanel.isCollapsed = !collapsiblePanel.isCollapsed
            }
        }

        Rectangle{
            id: contentContainer

            Layout.fillWidth: true
            Layout.preferredHeight: collapsiblePanel.isCollapsed ? 0 : 370 // ~+70 for every operation added

            color: Material.color(Material.BlueGrey)

            Layout.margins: 5

            visible: Layout.preferredHeight > 0

            radius: 5
            clip:true

            Behavior on Layout.preferredHeight {
                 NumberAnimation {
                     duration: 200
                     easing.type: Easing.InOutQuad
                 }
             }
        }
    }
}
