import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

Page {
    padding: 10

    header: Label {
        text: qsTr("Calibration checklist")
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ColumnLayout {
        anchors.fill: parent
        Label {
            Layout.fillWidth: true
            text: qsTr("Before calibrating the display, it is recommended to configure your display with the following settings to get optimal results.")
            wrapMode: "WordWrap"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("For best results, the display should have been powered for at least 15 minutes before starting the calibration.")
            wrapMode: "WordWrap"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        Item {
            Layout.fillHeight: true
        }
    }

}
