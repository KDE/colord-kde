/***************************************************************************
 *   Copyright (C) 2012 by Daniel Nicoletti                                *
 *   dantti12@gmail.com                                                    *
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

#include "ProfileModel.h"
#include "Profile.h"

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>
#include <QStringBuilder>

#include <KDebug>
#include <KLocale>
#include <KDateTime>
#include <KIcon>

DeviceModel::DeviceModel(QObject *parent) :
    QStandardItemModel(parent)
{
    qDBusRegisterMetaType<ObjectPathList>();

    // Creates a ColorD interface, it must be created with new
    // otherwise the object will be deleted when this block ends
    QDBusInterface *interface;
    interface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                   QLatin1String("/org/freedesktop/ColorManager"),
                                   QLatin1String("org.freedesktop.ColorManager"),
                                   QDBusConnection::systemBus(),
                                   this);
    // listen to colord for events
    connect(interface, SIGNAL(DeviceAdded(QDBusObjectPath)),
            this, SLOT(deviceAdded(QDBusObjectPath)));
    connect(interface, SIGNAL(DeviceRemoved(QDBusObjectPath)),
            this, SLOT(deviceRemoved(QDBusObjectPath)));
    connect(interface, SIGNAL(DeviceChanged(QDBusObjectPath)),
            this, SLOT(deviceChanged(QDBusObjectPath)));

    // Ask for devices
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("GetDevices"));
    QDBusConnection::systemBus().callWithCallback(message, this, SLOT(gotDevices(QDBusMessage)));
}

void DeviceModel::gotDevices(const QDBusMessage &message)
{
    if (message.type() == QDBusMessage::ReplyMessage && message.arguments().size() == 1) {
        QDBusArgument argument = message.arguments().first().value<QDBusArgument>();
        ObjectPathList paths = qdbus_cast<ObjectPathList>(argument);
        foreach (const QDBusObjectPath &path, paths) {
            deviceAdded(path, false);
        }
        emit changed();
    } else {
        kWarning() << "Unexpected message" << message;
    }
}

void DeviceModel::deviceChanged(const QDBusObjectPath &objectPath)
{
    int row = findItem(objectPath);
    if (row == -1) {
        kWarning() << "Device not found" << objectPath.path();
        return;
    }

    QDBusInterface deviceInterface(QLatin1String("org.freedesktop.ColorManager"),
                                   objectPath.path(),
                                   QLatin1String("org.freedesktop.ColorManager.Device"),
                                   QDBusConnection::systemBus(),
                                   this);
    if (!deviceInterface.isValid()) {
        return;
    }

    ObjectPathList profiles = deviceInterface.property("Profiles").value<ObjectPathList>();

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
        kWarning() << "Device is already on the list" << objectPath.path();
        return;
    }

    QDBusInterface deviceInterface(QLatin1String("org.freedesktop.ColorManager"),
                                   objectPath.path(),
                                   QLatin1String("org.freedesktop.ColorManager.Device"),
                                   QDBusConnection::systemBus(),
                                   this);
    if (!deviceInterface.isValid()) {
        return;
    }

    QString deviceId = deviceInterface.property("DeviceId").toString();
    QString kind = deviceInterface.property("Kind").toString();
    QString model = deviceInterface.property("Model").toString();
    QString vendor = deviceInterface.property("Vendor").toString();
    QString colorspace = deviceInterface.property("Colorspace").toString();
    ObjectPathList profiles = deviceInterface.property("Profiles").value<ObjectPathList>();

    QStandardItem *item = new QStandardItem;
    item->setData(qVariantFromValue(objectPath), ObjectPathRole);
    item->setData(true, IsDeviceRole);

    if (kind == QLatin1String("display")) {
        item->setIcon(KIcon(QLatin1String("video-display")));
    } else if (kind == QLatin1String("scanner")) {
        item->setIcon(KIcon(QLatin1String("scanner")));
    } else if (kind == QLatin1String("printer")) {
        if (colorspace == QLatin1String("gray")) {
            item->setIcon(KIcon(QLatin1String("printer-laser")));
        } else {
            item->setIcon(KIcon(QLatin1String("printer")));
        }
    } else if (kind == QLatin1String("webcam")) {
        item->setIcon(KIcon(QLatin1String("camera-web")));
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
        kind = QLatin1String("display-device");
    } else if (kind == QLatin1String("camera") ||
               kind == QLatin1String("scanner") ||
               kind == QLatin1String("webcam")) {
        kind = QLatin1String("input-device");
    } else if (kind == QLatin1String("printer")) {
        kind = QLatin1String("output-device");
    } else {
        kind = QLatin1String("unknown");
    }
    item->setData(kind, ProfileKindRole);

    QList<QStandardItem*> profileItems;
    foreach (const QDBusObjectPath &profileObjectPath, profiles) {
        QStandardItem *profileItem = createProfileItem(profileObjectPath,
                                                       objectPath,
                                                       profileItems.isEmpty());
        if (profileItem) {
            profileItems << profileItem;
        }
    }
    item->appendRows(profileItems);

    appendRow(item);

    if (emitChanged) {
        emit changed();
    }
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

QStandardItem* DeviceModel::createProfileItem(const QDBusObjectPath &objectPath,
                                              const QDBusObjectPath &parentObjectPath,
                                              bool checked)
{
    QDBusInterface interface(QLatin1String("org.freedesktop.ColorManager"),
                             objectPath.path(),
                             QLatin1String("org.freedesktop.ColorManager.Profile"),
                             QDBusConnection::systemBus(),
                             this);
    if (!interface.isValid()) {
        return 0;
    }

    QStandardItem *stdItem = new QStandardItem;
    QString dataSource = ProfileModel::getProfileDataSource(objectPath);
    QString kind = interface.property("Kind").toString();
    QString filename = interface.property("Filename").toString();
    QString title = interface.property("Title").toString();
    qulonglong created = interface.property("Created").toULongLong();

    // Sets the profile title
    bool canRemoveProfile = true;
    if (title.isEmpty()) {
        QString colorspace = interface.property("Colorspace").toString();
        if (colorspace == QLatin1String("rgb")) {
            title = i18nc("colorspace", "Default RGB");
        } else if (colorspace == QLatin1String("cmyk")) {
            title = i18nc("colorspace", "Default CMYK");
        } else if (colorspace == QLatin1String("gray")) {
            title = i18nc("colorspace", "Default Gray");
        }
        canRemoveProfile = false;
    } else {
        KDateTime createdDT;
        createdDT.setTime_t(created);
        title = Profile::profileWithSource(dataSource, title, createdDT);

        if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_EDID)) {
            canRemoveProfile = false;
        }
    }
    stdItem->setText(title);
    stdItem->setData(canRemoveProfile, CanRemoveProfileRole);

    stdItem->setData(qVariantFromValue(objectPath), ObjectPathRole);
    stdItem->setData(qVariantFromValue(parentObjectPath), ParentObjectPathRole);
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
    return 0;
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
    QDBusMessage message;
    QDBusObjectPath parentObjPath = stdItem->data(ParentObjectPathRole).value<QDBusObjectPath>();
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             parentObjPath.path(),
                                             QLatin1String("org.freedesktop.ColorManager.Device"),
                                             QLatin1String("MakeProfileDefault"));
    message << stdItem->data(ObjectPathRole);
    QDBusConnection::systemBus().send(message);

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
