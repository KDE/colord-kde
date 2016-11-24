import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

ApplicationWindow {
    width: 300
    height: 400
    visible: true
    title: qsTr("Introduction")

    SwipeView {
        id: view
        anchors.fill: parent

        Introduction {

        }

        CheckSettings {

        }

        CalibrationQuality {

        }

        DisplayType {

        }

        ProfileTitle {

        }

        Action {
            id: action

            property bool current: SwipeView.isCurrentItem
            onCurrentChanged: {
                if (current) {
                    ColordHelper.start();
                }
            }
        }
    }

    PageIndicator {
        id: indicator

        count: view.count
        currentIndex: view.currentIndex

        anchors.bottom: view.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Connections {
        target: ColordHelper
        onInteraction: {
            console.debug(image)
            action.image = image
        }
    }

    footer: RowLayout {
        Layout.fillWidth: true
        Button {
            enabled: view.currentIndex > 0
            text: qsTr("Back")
            onClicked: view.currentIndex = view.currentIndex - 1
        }
        Button {
            enabled: view.currentIndex + 1 < view.count
            text: qsTr("Next")
            onClicked: view.currentIndex = view.currentIndex + 1
        }
    }
}
