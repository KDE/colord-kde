import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1

Page {
    padding: 10

    title: qsTr("Introduction")

    header: Label {
        text: qsTr("Calibrate your display")
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ColumnLayout {
        anchors.fill: parent
        Label {
            Layout.fillWidth: true
            text: qsTr("Any existing screen correction will be temporarily turned off and the brightness set to maximum.")
            wrapMode: "WordWrap"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("You can cancel this process at any stage by pressing the cancel button.")
            wrapMode: "WordWrap"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        Item {
            Layout.fillHeight: true
        }
    }

}
