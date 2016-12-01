import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1

Page {
    title: qsTr("Choose Display Type")

    header: Label {
        padding: 10
        text: qsTr("Choose your display type")
        font.pixelSize: 20
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    ColumnLayout {
        anchors.fill: parent

        Label {
            Layout.fillWidth: true
            padding: 10
            text: qsTr("Select the monitor type that is attached to your computer.")
            wrapMode: "WordWrap"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        ListView {
            Layout.fillHeight: true
            Layout.fillWidth: true

            clip: true
            model: model
            delegate: RadioButton {
                padding: 10
                text: label
                checked: model.checked
                onCheckedChanged: ColordHelper.displayType = value
            }
            ScrollBar.vertical: ScrollBar { }
        }
    }

    ListModel {
        id: model
        ListElement {
            checked: true
            label: qsTr("LCD (CCFL backlight)")
            value: 0
        }
        ListElement {
            label: qsTr("LCD (White LED backlight)")
            value: 1
        }
        ListElement {
            label: qsTr("LCD (RGB LED backlight)")
            value: 2
        }
        ListElement {
            label: qsTr("LCD (Wide Gamut RGB LED backlight)")
            value: 3
        }
        ListElement {
            label: qsTr("LCD (Wide Gamut CCFL backlight)")
            value: 4
        }
        ListElement {
            label: qsTr("CRT")
            value: 5
        }
        ListElement {
            label: qsTr("Plasma")
            value: 5
        }
        ListElement {
            label: qsTr("Projector")
            value: 6
        }
    }
}
