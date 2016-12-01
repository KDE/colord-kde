import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1

Page {
    padding: 10
    title: qsTr("Calibration quality")

    header: Label {
        text: qsTr("Choose calibration quality")
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ColumnLayout {
        anchors.fill: parent
        Label {
            Layout.fillWidth: true
            text: qsTr("Higher quality calibration requires many color samples and more time.")
            wrapMode: "WordWrap"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        RadioButton {
            text: qsTr("Accurate (about 60 minutes)")
            onCheckedChanged: ColordHelper.quality = 2
        }
        RadioButton {
            text: qsTr("Normal (about 40 minutes)")
            onCheckedChanged: ColordHelper.quality = 1
        }
        RadioButton {
            checked: true
            text: qsTr("Quick (about 20 minutes)")
            onCheckedChanged: ColordHelper.quality = 0
        }

        Item {
            Layout.fillHeight: true
        }
    }

}
