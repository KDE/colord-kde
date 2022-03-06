/*
  SPDX-FileCopyrightLabel: 2022 Han Young <hanyoung@protonmail.com>

  SPDX-License-Identifier: LGPL-3.0-or-later
*/
import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.15 as Kirigami
import org.kde.kcm 1.2 as KCM
import colordkcm 1.0

KCM.ScrollViewKCM {
    header: QQC2.TabBar {
        width: parent.width
        QQC2.TabButton {
            text: i18n("Devices")
            icon.name: "systemsettings"
            onClicked: {
                deviceView.visible = true;
                view = deviceView;
                profileView.visible = false;
            }
        }
        QQC2.TabButton {
            Layout.alignment: Qt.AlignLeft
            text: i18n("Profiles")
            icon.name: "application-vnd.iccprofile"
            onClicked: {
                profileView.visible = true;
                view = profileView;
                deviceView.visible = false;
            }
        }
    }

    ListView {
        id: profileView
        model: kcm.profileModel
        delegate: Kirigami.BasicListItem {
            text: profileName
            icon: profileIcon
        }
    }

    view: ListView {
        id: deviceView
        model: kcm.deviceModel
        delegate: Kirigami.BasicListItem {
            text: deviceName
            icon: deviceIcon
        }
    }
}
