import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1

ApplicationWindow {
    id: mainWindow
    width: 300
    height: 400
    visible: true
    title: view.currentItem.title

    property bool sensorPresent: ColordHelper.sensorDetected
    property int maxCurrentIndex: 0
    onSensorPresentChanged: {
        console.debug(sensorPresent)
        if (sensorPresent) {
            if (view.currentIndex === 0) {
                view.currentIndex = 1
            }
        } else {
            view.currentIndex = 0
        }
    }

    SwipeView {
        id: view
        anchors.fill: parent
        onCurrentIndexChanged: {
            if (currentIndex > 0 && !sensorPresent) {
                currentIndex = 0
            }
        }

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

    SamplesWindow {
        id: samplesWindow
        visible: action.current
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
            onClicked: view.decrementCurrentIndex()
        }
        Button {
            enabled: view.currentIndex + 1 < view.count
            text: qsTr("Next")
            onClicked: view.incrementCurrentIndex()
        }
    }
}
