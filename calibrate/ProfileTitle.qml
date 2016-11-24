import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

Page {
    padding: 10

    header: Label {
        text: qsTr("Profile title")
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ColumnLayout {
        anchors.fill: parent
        Label {
            Layout.fillWidth: true
            text: qsTr("Choose a title to identify the profile on your system.")
            wrapMode: "WordWrap"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        TextField {
            Layout.fillWidth: true
            onTextChanged: ColordHelper.profileTitle = text
        }

        Item {
            Layout.fillHeight: true
        }
    }

}
