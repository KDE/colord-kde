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
    connect(interface, SIGNAL(DeviceChanged(QDBusObjectPath)),
            this, SLOT(deviceChanged(QDBusObjectPath)));


    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("GetDevices"));
    QDBusReply<ObjectPathList> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);
    foreach (const QDBusObjectPath &path, reply.value()) {
        kDebug() << path.path();
        deviceAdded(path);
    }
}

void DeviceModel::deviceChanged(const QDBusObjectPath &objectPath)
{

}

void DeviceModel::deviceAdded(const QDBusObjectPath &objectPath)
{
    QDBusInterface *deviceInterface;
    deviceInterface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                         objectPath.path(),
                                         QLatin1String("org.freedesktop.ColorManager.Device"),
                                         QDBusConnection::systemBus(),
                                         this);
    if (!deviceInterface->isValid()) {
        return;
    }

    QString deviceId = deviceInterface->property("DeviceId").toString();
    QString kind = deviceInterface->property("Kind").toString();
    QString model = deviceInterface->property("Model").toString();
    QString vendor = deviceInterface->property("Vendor").toString();
    QString colorspace = deviceInterface->property("Colorspace").toString();

    QStandardItem *item = new QStandardItem;
    item->setData(objectPath.path());
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

    appendRow(item);
}

QVariant DeviceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return i18n("Devices");
    }
    return QVariant();
}

Qt::ItemFlags DeviceModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

#include "DeviceModel.moc"
