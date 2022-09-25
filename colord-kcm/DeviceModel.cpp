/***************************************************************************
 *   Copyright (C) 2012-2016 by Daniel Nicoletti <dantti12@gmail.com>      *
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

#include "DeviceModel.h"

#include "Profile.h"
#include "ProfileModel.h"

#include "CdDeviceInterface.h"
#include "CdInterface.h"
#include "CdProfileInterface.h"

#include <QDateTime>
#include <QDebug>
#include <QIcon>

#include <KLocalizedString>

DeviceModel::DeviceModel(CdInterface *cdInterface, QObject *parent)
    : QStandardItemModel(parent)
    , m_cdInterface(cdInterface)
{
    qDBusRegisterMetaType<CdStringMap>();

    // listen to colord for events
    connect(m_cdInterface, &CdInterface::DeviceAdded, this, &DeviceModel::deviceAddedEmit);
    connect(m_cdInterface, &CdInterface::DeviceRemoved, this, &DeviceModel::deviceRemoved);
    connect(m_cdInterface, &CdInterface::DeviceChanged, this, &DeviceModel::deviceChanged);

    // Ask for devices
    auto async = m_cdInterface->GetDevices();
    auto watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &DeviceModel::gotDevices);
}

void DeviceModel::gotDevices(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<ObjectPathList> reply = *call;
    if (reply.isError()) {
        qWarning() << "Unexpected message" << reply.error().message();
    } else {
        const ObjectPathList devices = reply.argumentAt<0>();
        for (const QDBusObjectPath &device : devices) {
            deviceAdded(device, false);
        }
        emit changed();
    }
    call->deleteLater();
}

void DeviceModel::deviceChanged(const QDBusObjectPath &objectPath)
{
    int row = findItem(objectPath);
    if (row == -1) {
        qWarning() << "Device not found" << objectPath.path();
        return;
    }

    CdDeviceInterface device(QStringLiteral("org.freedesktop.ColorManager"), objectPath.path(), QDBusConnection::systemBus());
    if (!device.isValid()) {
        return;
    }

    const ObjectPathList profiles = device.profiles();

    // Normally just the profile list bound this device
    // is what changes including "Modified" property
    QStandardItem *stdItem = item(row);
    for (int i = 0; i < profiles.size(); ++i) {
        // Look for the desired profile
        QStandardItem *child = findProfile(stdItem, profiles.at(i));
        if (child) {
            // Check if the state has changed
            Qt::CheckState state = i ? Qt::Unchecked : Qt::Checked;
            if (child->checkState() != state) {
                child->setCheckState(state);
            }
        } else {
            // Inserts the profile with the parent object Path
            QStandardItem *profileItem = createProfileItem(profiles.at(i), objectPath, !i);
            if (profileItem) {
                stdItem->insertRow(i, profileItem);
            }
        }
    }

    // Remove the extra items it might have
    removeProfilesNotInList(stdItem, profiles);

    emit changed();
}

void DeviceModel::deviceAdded(const QDBusObjectPath &objectPath, bool emitChanged)
{
    if (findItem(objectPath) != -1) {
        qWarning() << "Device is already on the list" << objectPath.path();
        return;
    }

    CdDeviceInterface device(QStringLiteral("org.freedesktop.ColorManager"), objectPath.path(), QDBusConnection::systemBus());
    if (!device.isValid()) {
        return;
    }

    const QString deviceId = device.deviceId();
    QString kind = device.kind();
    const QString model = device.model();
    const QString vendor = device.vendor();
    const QString colorspace = device.colorspace();
    const ObjectPathList profiles = device.profiles();

    QStandardItem *item = new QStandardItem;
    item->setData(QVariant::fromValue(objectPath), ObjectPathRole);
    item->setData(true, IsDeviceRole);

    if (kind == QLatin1String("display")) {
        item->setIcon(QIcon::fromTheme(QStringLiteral("video-display")));
    } else if (kind == QLatin1String("scanner")) {
        item->setIcon(QIcon::fromTheme(QStringLiteral("scanner")));
    } else if (kind == QLatin1String("printer")) {
        if (colorspace == QLatin1String("gray")) {
            item->setIcon(QIcon::fromTheme(QStringLiteral("printer-laser")));
        } else {
            item->setIcon(QIcon::fromTheme(QStringLiteral("printer")));
        }
    } else if (kind == QLatin1String("webcam")) {
        item->setIcon(QIcon::fromTheme(QStringLiteral("camera-web")));
    }

    if (model.isEmpty() && vendor.isEmpty()) {
        item->setText(deviceId);
    } else if (model.isEmpty()) {
        item->setText(vendor);
    } else if (vendor.isEmpty()) {
        item->setText(model);
    } else {
        item->setText(vendor % QLatin1String(" - ") % model);
    }

    item->setData(QString(kind % item->text()), SortRole);

    // Convert our Device Kind to Profile Kind
    if (kind == QLatin1String("display")) {
        kind = QStringLiteral("display-device");
    } else if (kind == QLatin1String("camera") || kind == QLatin1String("scanner") || kind == QLatin1String("webcam")) {
        kind = QStringLiteral("input-device");
    } else if (kind == QLatin1String("printer")) {
        kind = QStringLiteral("output-device");
    } else {
        kind = QStringLiteral("unknown");
    }
    item->setData(kind, ProfileKindRole);
    appendRow(item);

    QList<QStandardItem *> profileItems;
    for (const QDBusObjectPath &profileObjectPath : profiles) {
        QStandardItem *profileItem = createProfileItem(profileObjectPath, objectPath, profileItems.isEmpty());
        if (profileItem) {
            profileItems << profileItem;
        }
    }
    item->appendRows(profileItems);

    if (emitChanged) {
        emit changed();
    }
}

void DeviceModel::deviceAddedEmit(const QDBusObjectPath &objectPath)
{
    deviceAdded(objectPath);
}

void DeviceModel::deviceRemoved(const QDBusObjectPath &objectPath)
{
    int row = findItem(objectPath);
    if (row != -1) {
        removeRow(row);
    }

    emit changed();
}

void DeviceModel::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(serviceName)
    if (newOwner.isEmpty() || oldOwner != newOwner) {
        // colord has quit or restarted
        removeRows(0, rowCount());
        emit changed();
    }
}

QStandardItem *DeviceModel::createProfileItem(const QDBusObjectPath &objectPath, const QDBusObjectPath &parentObjectPath, bool checked)
{
    CdProfileInterface profile(QStringLiteral("org.freedesktop.ColorManager"), objectPath.path(), QDBusConnection::systemBus());
    if (!profile.isValid()) {
        return nullptr;
    }

    QStandardItem *stdItem = new QStandardItem;
    const QString dataSource = ProfileModel::getProfileDataSource(profile.metadata());
    const QString kind = profile.kind();
    const QString filename = profile.filename();
    QString title = profile.title();
    const qlonglong created = profile.created();

    // Sets the profile title
    bool canRemoveProfile = true;
    if (title.isEmpty()) {
        QString colorspace = profile.colorspace();
        if (colorspace == QLatin1String("rgb")) {
            title = i18nc("colorspace", "Default RGB");
        } else if (colorspace == QLatin1String("cmyk")) {
            title = i18nc("colorspace", "Default CMYK");
        } else if (colorspace == QLatin1String("gray")) {
            title = i18nc("colorspace", "Default Gray");
        }
        canRemoveProfile = false;
    } else {
        QDateTime createdDT;
        createdDT.setTime_t(created);
        title = Profile::profileWithSource(dataSource, title, createdDT);

        if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_EDID)) {
            canRemoveProfile = false;
        }
    }
    stdItem->setText(title);
    stdItem->setData(canRemoveProfile, CanRemoveProfileRole);

    stdItem->setData(QVariant::fromValue(objectPath), ObjectPathRole);
    stdItem->setData(QVariant::fromValue(parentObjectPath), ParentObjectPathRole);
    stdItem->setData(filename, FilenameRole);
    stdItem->setData(kind, ProfileKindRole);
    stdItem->setData(QString(ProfileModel::getSortChar(kind) % title), SortRole);
    stdItem->setCheckable(true);
    stdItem->setCheckState(checked ? Qt::Checked : Qt::Unchecked);

    return stdItem;
}

QStandardItem *DeviceModel::findProfile(QStandardItem *parent, const QDBusObjectPath &objectPath)
{
    QStandardItem *child;
    for (int i = 0; i < parent->rowCount(); ++i) {
        child = parent->child(i);
        if (child->data(ObjectPathRole).value<QDBusObjectPath>() == objectPath) {
            return child;
        }
    }
    return nullptr;
}

void DeviceModel::removeProfilesNotInList(QStandardItem *parent, const ObjectPathList &profiles)
{
    QStandardItem *child;
    for (int i = 0; i < parent->rowCount(); ++i) {
        child = parent->child(i);
        // If the profile object path is not on the list remove it
        if (!profiles.contains(child->data(ObjectPathRole).value<QDBusObjectPath>())) {
            parent->removeRow(i);
            // i index is now pointing to a different value
            // we need to go back so we don't skip an item
            --i;
        }
    }
}

int DeviceModel::findItem(const QDBusObjectPath &objectPath)
{
    for (int i = 0; i < rowCount(); ++i) {
        if (item(i)->data(ObjectPathRole).value<QDBusObjectPath>() == objectPath) {
            return i;
        }
    }
    return -1;
}

QVariant DeviceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return i18n("Devices");
    }
    return QVariant();
}

bool DeviceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value)
    Q_UNUSED(role)

    QStandardItem *stdItem = itemFromIndex(index);
    QDBusObjectPath parentObjPath = stdItem->data(ParentObjectPathRole).value<QDBusObjectPath>();
    CdDeviceInterface device(QStringLiteral("org.freedesktop.ColorManager"), parentObjPath.path(), QDBusConnection::systemBus());
    if (device.isValid()) {
        device.MakeProfileDefault(stdItem->data(ObjectPathRole).value<QDBusObjectPath>());
    }

    // We return false since colord will emit a DeviceChanged signal telling us about this change
    return false;
}

Qt::ItemFlags DeviceModel::flags(const QModelIndex &index) const
{
    QStandardItem *stdItem = itemFromIndex(index);
    if (stdItem && stdItem->isCheckable() && stdItem->checkState() == Qt::Unchecked) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    }
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}
