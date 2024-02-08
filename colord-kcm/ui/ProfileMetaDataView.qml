/*
  SPDX-FileCopyrightLabel: 2022 Han Young <hanyoung@protonmail.com>

  SPDX-License-Identifier: LGPL-3.0-or-later
*/
import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kcmutils as KCM
import org.kde.kirigami 2.15 as Kirigami
import org.kde.kirigami.delegates as KD
import kcmcolord 1

ColumnLayout {
    id: profileDescriptionView

    QQC2.TabBar {
        id: tabBar
        QQC2.TabButton {
            id: informationTab
            text: i18nd("colord-kde", "Information")
            onClicked: {
                metaDataView.visible = false;
                informationView.visible = true;
                namedColorView.visible = false;
            }
        }
        QQC2.TabButton {
            text: i18nd("colord-kde", "Metadata")
            onClicked: {
                metaDataView.visible = true;
                informationView.visible = false;
                namedColorView.visible = false;
            }
        }
        QQC2.TabButton {
            text: i18ndc("colord-kde", "button: tab button for 'named colors in profile' information page", "Named Colors")
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
        delegate: QQC2.ItemDelegate {
            id: delegate

            text: name
            width: ListView.view.width

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                KD.TitleSubtitle {
                    title: delegate.text
                    subtitle: colorValue
                    Layout.fillWidth: true
                }
                Rectangle {
                    color: colorValue
                    implicitWidth: height
                    Layout.fillHeight: true
                }
            }
        }
    }

    ColumnLayout {
        id: informationView
        Layout.fillHeight: true
        Layout.fillWidth: true
        RowLayout {
            QQC2.Label {
                text: i18nd("colord-kde", "Profile Type: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.kind
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18nd("colord-kde", "Colorspace: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.colorSpace
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18nd("colord-kde", "Created: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.createdTime
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18nd("colord-kde", "Version: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.version
            }
        }
        RowLayout {
            Layout.fillWidth: true
            QQC2.Label {
                text: i18nd("colord-kde", "Device Model: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.model
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18nd("colord-kde", "Display Correction: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.hasDisplayCorrection ? i18nd("colord-kde", "Yes") : i18nd("colord-kde", "None")
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18nd("colord-kde", "White Point: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.whitePoint
            }
        }
        RowLayout {
            Layout.fillWidth: true
            QQC2.Label {
                text: i18nd("colord-kde", "License: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.license
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18nd("colord-kde", "File Size: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.size
            }
        }
        RowLayout {
            QQC2.Label {
                text: i18nd("colord-kde", "Filename: ")
            }
            QQC2.Label {
                text: kcm.profileDescription.filename
            }
        }

        QQC2.Button {
            Layout.alignment: Qt.AlignHCenter
            text: i18nd("colord-kde", "Install System Wide")
            visible: kcm.profileDescription.canRemove
            onClicked: kcm.profileDescription.installSystemWide()
        }
    }
}
