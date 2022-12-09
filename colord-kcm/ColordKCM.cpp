/***************************************************************************
 *   Copyright (C) 2012 by Daniel Nicoletti                                *
 *   dantti12@gmail.com                                                    *
 *   2022 by Han Young <hanyoung@protonmail.com                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; see the file COPYING. If not, write to       *
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,  *
 *   Boston, MA 02110-1301, USA.                                           *
 ***************************************************************************/

#include <set>

#include <KPluginFactory>
#include <KSharedConfig>
#include <klocalizedstring.h>

#include "CdDeviceInterface.h"
#include "CdInterface.h"
#include "CdProfileInterface.h"

#include "ColordKCM.h"
#include "DeviceDescription.h"
#include "DeviceModel.h"
#include "ProfileDescription.h"
#include "ProfileMetaDataModel.h"
#include "ProfileModel.h"
#include "ProfileNamedColorsModel.h"

#include "qdbusconnection.h"
K_PLUGIN_CLASS_WITH_JSON(KCMColord, "kcm_colord.json")

KCMColord::KCMColord(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ConfigModule(parent, data, args)
    , m_cdInterface(
          new CdInterface(QStringLiteral("org.freedesktop.ColorManager"), QStringLiteral("/org/freedesktop/ColorManager"), QDBusConnection::systemBus(), this))
    , m_deviceModel(new DeviceModel(m_cdInterface, this))
    , m_profileModel(new ProfileModel(m_cdInterface, this))
    , m_deviceDescription(new DeviceDescription(this))
    , m_profileDescription(new ProfileDescription(this))
    , m_filterModel(new QSortFilterProxyModel(this))
{
    qmlRegisterAnonymousType<DeviceModel>("kcmcolord", 1);
    qmlRegisterAnonymousType<ProfileModel>("kcmcolord", 1);
    qmlRegisterAnonymousType<ProfileMetaDataModel>("kcmcolord", 1);
    qmlRegisterAnonymousType<ProfileNamedColorsModel>("kcmcolord", 1);
    qmlRegisterAnonymousType<DeviceDescription>("kcmcolord", 1);
    qmlRegisterAnonymousType<ProfileDescription>("kcmcolord", 1);
    qmlRegisterAnonymousType<AddProfileComboBoxItem>("kcmcolord", 1);
    // Make sure we know is colord is running
    auto watcher =
        new QDBusServiceWatcher(QStringLiteral("org.freedesktop.ColorManager"), QDBusConnection::systemBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, m_deviceModel, &DeviceModel::serviceOwnerChanged);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, m_profileModel, &ProfileModel::serviceOwnerChanged);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, m_deviceDescription, &DeviceDescription::serviceOwnerChanged);
}

ProfileModel *KCMColord::profileModel() const
{
    return m_profileModel;
}

DeviceModel *KCMColord::deviceModel() const
{
    return m_deviceModel;
}

DeviceDescription *KCMColord::deviceDescription() const
{
    return m_deviceDescription;
}

ProfileDescription *KCMColord::profileDescription() const
{
    return m_profileDescription;
}

void KCMColord::assignProfileToDevice(const QDBusObjectPath &profile, const QDBusObjectPath &devicePath) const
{
    CdDeviceInterface device(QStringLiteral("org.freedesktop.ColorManager"), devicePath.path(), QDBusConnection::systemBus());
    if (device.isValid()) {
        device.AddProfile(QStringLiteral("hard"), profile);
    }
}

void KCMColord::makeProfileDefault(const QDBusObjectPath &profilePath, const QDBusObjectPath &devicePath)
{
    CdDeviceInterface device(QStringLiteral("org.freedesktop.ColorManager"), devicePath.path(), QDBusConnection::systemBus());
    if (device.isValid()) {
        device.MakeProfileDefault(profilePath);
    }
}

void KCMColord::importProfile(const QUrl &filepath)
{
    QProcess::execute(QStringLiteral("colord-kde-icc-importer"), {QStringLiteral("--yes"), filepath.path()});
}

void KCMColord::removeProfile(const QString &filename)
{
    // TODO: capture error and display it
    QFile::remove(filename);
}

bool KCMColord::canAddProfileForDevice(const QDBusObjectPath &device)
{
    auto index = m_deviceModel->findDeviceIndex(device);
    if (index < 0) {
        return {};
    }
    auto modelIndex = m_deviceModel->index(index, 0);
    auto colorspace = m_deviceModel->data(modelIndex, DeviceModel::ColorspaceRole).toString();
    auto profileKind = m_deviceModel->data(modelIndex, DeviceModel::ProfileKindRole).toString();
    auto item = m_deviceModel->item(index);
    if (item) {
        auto rowCount = item->rowCount();
        std::set<QDBusObjectPath> assigned;
        for (int i = 0; i < rowCount; ++i) {
            assigned.insert(item->child(i, 0)->data(DeviceModel::ObjectPathRole).value<QDBusObjectPath>());
        }

        auto profileCount = m_profileModel->rowCount();
        for (int i = 0; i < profileCount; i++) {
            auto pColorspace = m_profileModel->data(m_profileModel->index(i, 0), ProfileModel::ColorspaceRole).toString();
            auto pProfileKind = m_profileModel->data(m_profileModel->index(i, 0), ProfileModel::ProfileKindRole).toString();
            if (pColorspace != colorspace || pProfileKind != profileKind) {
                continue;
            }
            auto objectPath = m_profileModel->data(m_profileModel->index(i, 0), ProfileModel::ObjectPathRole).value<QDBusObjectPath>();
            if (assigned.count(objectPath)) {
                continue;
            }

            return true;
        }
    }
    return false;
}

QList<QObject *> KCMColord::getComboBoxItemsForDevice(const QDBusObjectPath &device)
{
    auto index = m_deviceModel->findDeviceIndex(device);
    if (index < 0) {
        return {};
    }

    auto modelIndex = m_deviceModel->index(index, 0);
    auto colorspace = m_deviceModel->data(modelIndex, DeviceModel::ColorspaceRole).toString();
    auto profileKind = m_deviceModel->data(modelIndex, DeviceModel::ProfileKindRole).toString();
    auto item = m_deviceModel->item(index);
    if (item) {
        auto rowCount = item->rowCount();
        std::set<QDBusObjectPath> assigned;
        for (int i = 0; i < rowCount; ++i) {
            assigned.insert(item->child(i, 0)->data(DeviceModel::ObjectPathRole).value<QDBusObjectPath>());
        }
        QList<QObject *> result;
        auto profileCount = m_profileModel->rowCount();
        for (int i = 0; i < profileCount; i++) {
            auto pColorspace = m_profileModel->data(m_profileModel->index(i, 0), ProfileModel::ColorspaceRole).toString();
            auto pProfileKind = m_profileModel->data(m_profileModel->index(i, 0), ProfileModel::ProfileKindRole).toString();
            if (pColorspace != colorspace || pProfileKind != profileKind) {
                continue;
            }
            auto objectPath = m_profileModel->data(m_profileModel->index(i, 0), ProfileModel::ObjectPathRole).value<QDBusObjectPath>();
            if (assigned.count(objectPath)) {
                continue;
            }
            auto name = m_profileModel->data(m_profileModel->index(i, 0), Qt::DisplayRole).toString();

            result.append(new AddProfileComboBoxItem(this, objectPath, name));
        }

        for (auto obj : std::as_const(m_comboBoxItemsToBeDestroyed)) {
            obj->deleteLater();
        }
        m_comboBoxItemsToBeDestroyed = result;

        return result;
    }
    return {};
}

#include "ColordKCM.moc"
#include "moc_ColordKCM.cpp"
