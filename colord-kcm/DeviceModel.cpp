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

#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusReply>
#include <QStringBuilder>

#include <KDebug>
#include <KLocale>
#include <KMessageBox>

typedef QList<QDBusObjectPath> ObjectPathList;

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

    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("GetDevices"));
    QDBusReply<ObjectPathList> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);
    foreach (const QDBusObjectPath &path, reply.value()) {
        deviceAdded(path);
    }
}

void DeviceModel::deviceChanged(const QDBusObjectPath &objectPath)
{
    int row = findItem(objectPath);
    if (row == -1) {
        kWarning() << "Device not found" << objectPath.path();
        return;
    }

    QDBusInterface *deviceInterface;
    deviceInterface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                         objectPath.path(),
                                         QLatin1String("org.freedesktop.ColorManager.Device"),
                                         QDBusConnection::systemBus(),
                                         this);
    if (!deviceInterface->isValid()) {
        deviceInterface->deleteLater();
        return;
    }

    ObjectPathList profiles = deviceInterface->property("Profiles").value<ObjectPathList>();
    deviceInterface->deleteLater();

    // Normally just the profile list bound this device
    // is what changes including "Modified" property
    QStandardItem *stdItem = item(row);
    for (int i = 0; i < profiles.size(); ++i) {
        QStandardItem *child;
        child = stdItem->child(i);
        if (child) {
            // If the profile item is the same ignore since
            // Profiles don't actually change
            if (child->data(ObjectPathRole).value<QDBusObjectPath>() == profiles.at(i)) {
                // Check if the state has changed
                Qt::CheckState state = i ? Qt::Unchecked : Qt::Checked;
                if (child->checkState() != state) {
                    child->setCheckState(state);
                }
                continue;
            }
            // Removes the item since it's not the one we are looking for
            stdItem->removeRow(i);
        }
        // Inserts the profile with the parent object Path
        stdItem->insertRow(i, createProfileItem(profiles.at(i), objectPath, !i));
    }

    // Remove the extra items it might have
    removeRows(profiles.size(), stdItem->rowCount() - profiles.size(), stdItem->index());
}

void DeviceModel::deviceAdded(const QDBusObjectPath &objectPath)
{
    if (findItem(objectPath) != -1) {
        kWarning() << "Device is already on the list" << objectPath.path();
        return;
    }

    QDBusInterface *deviceInterface;
    deviceInterface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                         objectPath.path(),
                                         QLatin1String("org.freedesktop.ColorManager.Device"),
                                         QDBusConnection::systemBus(),
                                         this);
    if (!deviceInterface->isValid()) {
        deviceInterface->deleteLater();
        return;
    }

    QString deviceId = deviceInterface->property("DeviceId").toString();
    QString kind = deviceInterface->property("Kind").toString();
    QString model = deviceInterface->property("Model").toString();
    QString vendor = deviceInterface->property("Vendor").toString();
    QString colorspace = deviceInterface->property("Colorspace").toString();
    ObjectPathList profiles = deviceInterface->property("Profiles").value<ObjectPathList>();
    deviceInterface->deleteLater();

    QStandardItem *item = new QStandardItem;
    item->setData(qVariantFromValue(objectPath), ObjectPathRole);

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

    item->setData(qVariantFromValue(kind + item->text()), SortRole);

    QList<QStandardItem*> profileItems;
    foreach (const QDBusObjectPath &profileObjectPath, profiles) {
        QStandardItem *profileItem = createProfileItem(profileObjectPath,
                                                       objectPath,
                                                       profileItems.isEmpty());
        profileItems << profileItem;
        kDebug() << profileObjectPath.path();
    }
    item->appendRows(profileItems);

    appendRow(item);
}

void DeviceModel::deviceRemoved(const QDBusObjectPath &objectPath)
{
    int row = findItem(objectPath);
    if (row != -1) {
        removeRow(row);
    }
}

QStandardItem* DeviceModel::createProfileItem(const QDBusObjectPath &objectPath,
                                              const QDBusObjectPath &parentObjectPath,
                                              bool checked)
{
    QDBusInterface *interface;
    interface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                   objectPath.path(),
                                   QLatin1String("org.freedesktop.ColorManager.Profile"),
                                   QDBusConnection::systemBus(),
                                   this);
    QStandardItem *stdItem = new QStandardItem;
    QString title = interface->property("Title").toString();
    if (title.isEmpty()) {
        QString colorspace = interface->property("Colorspace").toString();
        if (colorspace == QLatin1String("rgb")) {
            title = i18n("Default RGB");
        } else if (colorspace == QLatin1String("cmyk")) {
            title = i18n("Default CMYK");
        } else if (colorspace == QLatin1String("gray")) {
            title = i18n("Default Gray");
        }
    }

    stdItem->setText(title);
    stdItem->setData(qVariantFromValue(objectPath), ObjectPathRole);
    stdItem->setData(qVariantFromValue(parentObjectPath), ParentObjectPathRole);
    stdItem->setData(title, SortRole);
    stdItem->setCheckable(true);
    stdItem->setCheckState(checked ? Qt::Checked : Qt::Unchecked);

    interface->deleteLater();
    return stdItem;
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

#include "DeviceModel.moc"
