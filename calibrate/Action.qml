import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

Page {
    padding: 10
    title: qsTr("Action")

    property bool interactionRequired: false

    header: Label {
        id: titleLabel
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    Connections {
        target: ColordHelper
        onInteraction: {
            console.debug("code:" + code + " image:" + image)
            var text;
            if (code === 0) {
                // Attach
                titleLabel.text = qsTr("Please attach instrument")
                if (image.empty) {
                    text = qsTr("Please attach the measuring instrument to the center of the screen on the gray square.")
                    imageItem.source = ""
                } else {
                    text = qsTr("Please attach the measuring instrument to the center of the screen on the gray square like the image below.")
                    imageItem.source = "file://" + image
                }
                text = text + "\n\n" + qsTr("You will need to hold the device on the screen for the duration of the calibration.")
            } else if (code === 1) {
                // move to calibration mode
                titleLabel.text = qsTr("Please configure instrument")
                if (image.empty) {
                    text = qsTr("Please set the measuring instrument to screen mode.")
                    imageItem.source = ""
                } else {
                    text = qsTr("Please set the measuring instrument to screen mode like the image below.")
                    imageItem.source = "file://" + image
                }
            } else if (code === 2) {
                // move to surface mode
                titleLabel.text = qsTr("Please configure instrument")
                if (image.empty) {
                    text = qsTr("Please set the measuring instrument to calibration mode.")
                    imageItem.source = ""
                } else {
                    text = qsTr("Please set the measuring instrument to calibration mode like the image below.")
                    imageItem.source = "file://" + image
                }
            } else if (code === 3) {
                // shut the laptop lid
                titleLabel.text = qsTr("Please configure instrument")
                if (image.empty) {
                    text = qsTr("Please set the measuring instrument to calibration mode.")
                    imageItem.source = ""
                } else {
                    text = qsTr("Please set the measuring instrument to calibration mode like the image below.")
                    imageItem.source = "file://" + image
                }
            }
            message.text = text

            interactionRequired = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        Label {
            Layout.fillWidth: true
            id: message
            wrapMode: "WordWrap"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Image {
            Layout.fillHeight: true
            Layout.fillWidth: true
            id: imageItem
            fillMode: Image.PreserveAspectFit
        }
    }

}
