import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

Page {
    property string image

    padding: 10
    title: qsTr("Action")

    header: Label {
        text: qsTr("Please attach instrument")
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ColumnLayout {
        anchors.fill: parent
        Label {
            Layout.fillWidth: true
            text: qsTr("Please attach the measuring instrument to the center of the screen on the gray square like the image below.")
            wrapMode: "WordWrap"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("You will need to hold the device on the screen for the duration of the calibration.")
            wrapMode: "WordWrap"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Image {
            Layout.fillHeight: true
            Layout.fillWidth: true
            fillMode: Image.PreserveAspectFit
            source: "file:///usr/share/colord/icons/colorhug-attach.svg" + image
        }
    }

}
