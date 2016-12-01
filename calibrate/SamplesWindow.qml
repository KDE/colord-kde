import QtQuick 2.5
import QtQuick.Window 2.2

Window {
    width: 200
    height: 300
    x: Screen.desktopAvailableWidth / 2 - width / 2
    y: Screen.desktopAvailableHeight / 2 - height / 2

    flags: Qt.WindowStaysOnTopHint | Qt.X11BypassWindowManagerHint

    Rectangle {
        anchors.fill: parent
        color: "grey"
    }
}
