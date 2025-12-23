import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panelHeader

    property string title: "Panel"
    property bool isCollapsed: false
    color: Material.color(Material.Teal)


    signal toggleCollapse()

    height: 40

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Text {
            text: panelHeader.title
            font.bold: true
            font.pixelSize: 13
            color: "white"
            Layout.fillWidth: true
        }

        Button {
            id: collapseButton
            flat: true
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24

            Text {
                anchors.centerIn: parent
                text: panelHeader.isCollapsed ? "▶" : "▼"
                font.pixelSize: 11
                color: "white"
            }

            onClicked: {
                panelHeader.toggleCollapse()
            }
        }
    }
}
