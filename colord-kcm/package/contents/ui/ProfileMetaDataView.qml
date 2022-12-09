/*
  SPDX-FileCopyrightLabel: 2022 Han Young <hanyoung@protonmail.com>

  SPDX-License-Identifier: LGPL-3.0-or-later
*/
import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kcm 1.2 as KCM
import org.kde.kirigami 2.15 as Kirigami
import kcmcolord 1

ColumnLayout {
    id: profileDescriptionView

    QQC2.TabBar {
        id: tabBar
        QQC2.TabButton {
            id: informationTab
            text: i18n("Information")
            onClicked: {
                metaDataView.visible = false;
                informationView.visible = true;
                namedColorView.visible = false;
            }
        }
        QQC2.TabButton {
            text: i18n("Metadata")
            onClicked: {
                metaDataView.visible = true;
                informationView.visible = false;
                namedColorView.visible = false;
            }
        }
        QQC2.TabButton {
            text: i18nc("button: tab button for 'named colors in profile' information page", "Named Colors")
            onClicked: {
                metaDataView.visible = false;
                informationView.visible = false;
                namedColorView.visible = true;
            }
            visible: namedColorView.count > 0
        }
    }

    Connections {
        target: kcm.profileDescription
        function onPathChanged() {
            metaDataView.visible = false;
            informationView.visible = true;
            namedColorView.visible = false;
            informationTab.checked = true;
        }
    }

    ListView {
        id: metaDataView
        visible: false
        Layout.fillHeight: true
        Layout.fillWidth: true
        model: kcm.profileDescription.metaDataModel
        delegate: RowLayout {
            Text {
                text: title
            }
            Text {
                text: textValue
            }
        }
    }

    ListView {
        id: namedColorView
        clip: true
        visible: false
        Layout.fillHeight: true
        Layout.fillWidth: true
        model: kcm.profileDescription.namedColorsModel
        delegate: Kirigami.BasicListItem {
            label: name
            textColor: model.isDarkColor ? "#EEEDE7" : "#171710"
            subtitle: colorValue
            backgroundColor: colorValue
            separatorVisible: false
        }
    }

    ColumnLayout {
        id: informationView
        Layout.fillHeight: true
        Layout.fillWidth: true
        RowLayout {
            QQC2.Label {
                text: i18n("Profile Type: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.kind
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18n("Colorspace: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.colorSpace
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18n("Created: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.createdTime
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18n("Version: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.version
            }
        }
        RowLayout {
            Layout.fillWidth: true
            QQC2.Label {
                text: i18n("Device Model: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.model
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18n("Display Correction: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.hasDisplayCorrection ? i18n("Yes") : i18n("None")
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18n("White Point: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.whitePoint
            }
        }
        RowLayout {
            Layout.fillWidth: true
            QQC2.Label {
                text: i18n("License: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.license
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18n("File Size: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.size
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18n("Filename: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.filename
            }
        }

        QQC2.Button {
            Layout.alignment: Qt.AlignHCenter
            text: i18n("Install System Wide")
            visible: kcm.profileDescription.canRemove
            onClicked: kcm.profileDescription.installSystemWide()
        }
    }
}
