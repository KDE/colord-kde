/*
  SPDX-FileCopyrightLabel: 2022 Han Young <hanyoung@protonmail.com>

  SPDX-License-Identifier: LGPL-3.0-or-later
*/
import QtQuick
import QtCore
import QtQuick.Controls as QQC2
import QtQuick.Layouts 1.15
import org.kde.kitemmodels 1.0
import org.kde.kirigami 2.15 as Kirigami
import org.kde.kirigami.delegates as KD
import org.kde.kcmutils as KCM
import QtQuick.Dialogs as QtDialogs
import kcmcolord 1

KCM.AbstractKCM {
    id: root
    implicitHeight: Kirigami.Units.gridUnit * 40
    implicitWidth: Kirigami.Units.gridUnit * 20
    property bool isDeviceView: true
    property bool isDeviceDescription: true

    onIsDeviceViewChanged: {
        if (isDeviceView) {
            kcm.deviceDescription.setDevice(deviceView.currentDevicePath);
            kcm.profileDescription.setProfile(deviceView.currentProfilePath, deviceView.canRemoveCurrentProfile);
        } else {
            kcm.profileDescription.setProfile(profileView.currentProfilePath, profileView.canRemoveCurrentProfile);
        }
    }

    header: QQC2.ToolBar {
        Row {
            spacing: Kirigami.Units.largeSpacing
            anchors.fill: parent

            QQC2.ButtonGroup {
                id: headerGroup
            }

            QQC2.ToolButton {
                text: i18nd("colord-kde", "Devices")
                icon.name: "video-display"
                checkable: true
                checked: true
                display: QQC2.AbstractButton.TextUnderIcon
                onClicked: isDeviceView = true
                QQC2.ButtonGroup.group: headerGroup
            }
            QQC2.ToolButton {
                text: i18nd("colord-kde", "Profiles")
                icon.name: "color-management"
                checkable: true
                display: QQC2.AbstractButton.TextUnderIcon
                onClicked: isDeviceView = false
                QQC2.ButtonGroup.group: headerGroup
            }

            QQC2.ToolSeparator {
                implicitHeight: parent.height
            }

            QQC2.ToolButton {
                text: i18nd("colord-kde", "Import Profile")
                icon.name: "document-single"
                display: QQC2.AbstractButton.TextUnderIcon
                onClicked: fileDialog.open()
            }

            QQC2.ToolButton {
                text: i18nd("colord-kde", "Assign Profile")
                icon.name: "list-add"
                display: QQC2.AbstractButton.TextUnderIcon
                onClicked: assignProfileMenu.open()
                enabled: isDeviceDescription && assignProfileMenu.count > 0
                QQC2.Menu {
                    id: assignProfileMenu
                }

                Connections {
                    target: deviceView
                    function onCurrentDevicePathChanged() {
                        let oldActionsCount = assignProfileMenu.count;
                        while (oldActionsCount--) {
                            assignProfileMenu.takeItem(0);
                        }

                        let items = kcm.getComboBoxItemsForDevice(deviceView.currentDevicePath);
                        for (let i = 0; i < items.length; i++) {
                            let action = Qt.createQmlObject(`
                                                            import QtQuick.Controls 2.15
                                                            Action {
                                                            }
                                                            `,
                                                            assignProfileMenu,
                                                            "main.qml"
                                                            );
                            action.text = items[i].profileName;
                            action.triggered.connect((a) => {
                                kcm.assignProfileToDevice(items[i].objectPath, deviceView.currentDevicePath);
                            });
                            assignProfileMenu.addAction(action);
                        }
                    }
                }
            }

            QQC2.ToolButton {
                enabled: (isDeviceView && deviceView.canRemoveCurrentProfile) || (!isDeviceView && profileView.canRemoveCurrentProfile)
                text: i18nd("colord-kde", "Remove Profile")
                icon.name: "list-remove-symbolic"
                display: QQC2.AbstractButton.TextUnderIcon
                onClicked: removeProfileDialog.open()
            }
        }
    }

    QtDialogs.FileDialog {
        id: fileDialog
        title: i18ndc("colord-kde", "ICC Profile is a tech term", "Import ICC Profile")
        currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        onAccepted: {
            kcm.importProfile(selectedFile);
        }
        nameFilters: [i18ndc("colord-kde", "ICC Profile is a tech term", "ICC Profile") + "(*.icc)"]
    }

    QtDialogs.MessageDialog {
        id: removeProfileDialog
        title: i18nd("colord-kde", "Remove Profile")

        text: i18nd("colord-kde", "Are you sure you want to remove this profile?")
        buttons: QtDialogs.StandardButton.Yes | QtDialogs.StandardButton.No
        onAccepted: {
            if (isDeviceView) {
                kcm.deviceModel.removeProfile(deviceView.currentProfilePath, deviceView.currentDevicePath);
            } else {
                kcm.removeProfile(profileView.currentProfilePath);
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        QQC2.Frame {
            clip: true
            id: profileDeviceViewFrame
            Layout.fillWidth: true
            Layout.fillHeight: true

            QQC2.ScrollView {

                anchors.fill: parent

                TreeView {
                    property variant currentProfilePath: null
                    property variant currentDevicePath: null
                    property bool canRemoveCurrentProfile: false

                    id: deviceView
                    visible: isDeviceView
                    anchors.fill: parent

                    selectionModel: ItemSelectionModel {
                        id: selectionModel
                        model: kcm.deviceModel
                    }

                    model: kcm.deviceModel
                    delegate: QQC2.TreeViewDelegate {
                        id: listItem
                        implicitWidth: deviceView.width
                        contentItem: RowLayout {
                            id: layout
                            spacing: Kirigami.Units.smallSpacing

                            QQC2.CheckBox {
                                visible: itemType === "profile"
                                checkState: model.profileCheckState // from model data
                                checkable: !checked
                                onCheckedChanged: {
                                    if (checked) {
                                        kcm.makeProfileDefault(model.objectPath, model.parentObjectPath);
                                    }
                                }
                            }

                            KD.IconTitleSubtitle {
                                title: model.title
                                icon.name: model.iconName

                                Layout.fillWidth: true
                            }
                        }

                        onCurrentChanged: {
                            if (current) {
                                if (itemType === "device") {
                                    // is device
                                    kcm.deviceDescription.setDevice(objectPath);
                                    isDeviceDescription = true;

                                    deviceView.currentDevicePath = model.objectPath;
                                    deviceView.currentProfilePath = null;
                                    deviceView.canRemoveCurrentProfile = false;
                                } else {
                                    // is profile
                                    kcm.profileDescription.setProfile(objectPath, model.canRemoveProfile);
                                    isDeviceDescription = false;
                                    deviceView.currentDevicePath = model.parentObjectPath;
                                    deviceView.currentProfilePath = model.objectPath;
                                    deviceView.canRemoveCurrentProfile = model.canRemoveProfile;
                                }
                            }
                        }
                    }
                }
            }

            QQC2.ScrollView {
                visible: !isDeviceView
                anchors.fill: parent

                Component.onCompleted: background.visible = true

                ListView {
                    id: profileView

                    property variant currentProfilePath: null
                    property bool canRemoveCurrentProfile: false

                    clip: true

                    model: profileSortModel
                    delegate: QQC2.ItemDelegate {
                        id: delegate

                        text: title
                        icon.name: iconName

                        width: ListView.view.width
                        highlighted: ListView.isCurrentItem

                        contentItem: KD.IconTitleSubtitle {
                            title: delegate.text
                            subtitle: fileName
                            icon: icon.fromControlsIcon(delegate.icon)
                        }

                        onClicked: ListView.view.currentIndex = index

                        ListView.onIsCurrentItemChanged: {
                            kcm.profileDescription.setProfile(model.objectPath, model.canRemove);
                            profileView.currentProfilePath = model.objectPath;
                            profileView.canRemoveCurrentProfile = model.canRemove;
                        }
                    }
                    KSortFilterProxyModel {
                        id: profileSortModel
                        sourceModel: kcm.profileModel
                        sortRoleName: "sortString"
                    }
                }
            }
        }

        QQC2.Frame {
            Layout.fillHeight: true
            Layout.maximumWidth: parent.width * 0.4
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            clip: true
            ColumnLayout {
                anchors.fill: parent
                ColumnLayout {
                    id: deviceDescriptionView
                    visible: isDeviceDescription && isDeviceView
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    QQC2.Label {
                        text: kcm.deviceDescription.deviceTitle
                        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.5
                    }
                    QQC2.Label {
                        text: kcm.deviceDescription.deviceKind
                        Layout.alignment: Qt.AlignLeft
                    }
                    RowLayout {
                        QQC2.Label {
                            text: i18nd("colord-kde", "Id: ")
                        }
                        QQC2.Label {
                            text: kcm.deviceDescription.deviceID
                        }
                    }
                    RowLayout {
                        QQC2.Label {
                            text: i18nd("colord-kde", "Scope: ")
                        }
                        QQC2.Label {
                            text: kcm.deviceDescription.deviceScope
                        }
                    }
                    RowLayout {
                        QQC2.Label {
                            text: i18nd("colord-kde", "Mode: ")
                        }
                        QQC2.Label {
                            text: kcm.deviceDescription.colorSpace
                        }
                    }
                    RowLayout {
                        QQC2.Label {
                            text: i18nd("colord-kde", "Current Profile: ")
                        }
                        QQC2.Label {
                            text: kcm.deviceDescription.currentProfileTitle
                        }
                    }

                    Kirigami.InlineMessage {
                        text: kcm.deviceDescription.calibrateTipMessage
                        Layout.fillWidth: true
                        visible: true
                    }
                }
                ProfileMetaDataView {
                    id: profileDescriptionView
                    visible: !isDeviceView || !isDeviceDescription
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
            }
        }
    }
}
