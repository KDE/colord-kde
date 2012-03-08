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

#include "ProfileModel.h"

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

ProfileModel::ProfileModel(QObject *parent) :
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
    connect(interface, SIGNAL(ProfileAdded(QDBusObjectPath)),
            this, SLOT(profileAdded(QDBusObjectPath)));
    connect(interface, SIGNAL(ProfileRemoved(QDBusObjectPath)),
            this, SLOT(profileRemoved(QDBusObjectPath)));
    connect(interface, SIGNAL(ProfileChanged(QDBusObjectPath)),
            this, SLOT(profileChanged(QDBusObjectPath)));

    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("GetProfiles"));
    QDBusReply<ObjectPathList> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);
    foreach (const QDBusObjectPath &path, reply.value()) {
        kDebug() << path.path();
        profileAdded(path);
    }
}

void ProfileModel::profileChanged(const QDBusObjectPath &objectPath)
{
    int row = findItem(objectPath);
    if (row == -1) {
        kWarning() << "Profile not found" << objectPath.path();
        return;
    }

    QDBusInterface *profileInterface;
    profileInterface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                         objectPath.path(),
                                         QLatin1String("org.freedesktop.ColorManager.Profile"),
                                         QDBusConnection::systemBus(),
                                         this);
    if (!profileInterface->isValid()) {
        return;
    }

    ObjectPathList profiles = profileInterface->property("Profiles").value<ObjectPathList>();

    // Normally just the profile list bound this profile
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

void ProfileModel::profileAdded(const QDBusObjectPath &objectPath)
{
    if (findItem(objectPath) != -1) {
        kWarning() << "Device is already on the list" << objectPath.path();
        return;
    }

    QDBusInterface *profileInterface;
    profileInterface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                         objectPath.path(),
                                         QLatin1String("org.freedesktop.ColorManager.Profile"),
                                         QDBusConnection::systemBus(),
                                         this);
    if (!profileInterface->isValid()) {
        return;
    }

    QString profileId = profileInterface->property("ProfileId").toString();
    QString title = profileInterface->property("Title").toString();
//    QString colorspace = profileInterface->property("Colorspace").toString();

    QStandardItem *item = new QStandardItem;
    item->setData(qVariantFromValue(objectPath), ObjectPathRole);


    if (title.isEmpty()) {
        item->setText(profileId);
    } else {
        item->setText(title);
    }

    appendRow(item);
}

void ProfileModel::profileRemoved(const QDBusObjectPath &objectPath)
{
    int row = findItem(objectPath);
    if (row != -1) {
        removeRow(row);
    }
}

QStandardItem* ProfileModel::createProfileItem(const QDBusObjectPath &objectPath,
                                              const QDBusObjectPath &parentObjectPath,
                                              bool checked)
{
    kDebug() << objectPath.path() << checked;
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
    stdItem->setCheckable(true);
    stdItem->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    return stdItem;
}

int ProfileModel::findItem(const QDBusObjectPath &objectPath)
{
    for (int i = 0; i < rowCount(); ++i) {
        if (item(i)->data(ObjectPathRole).value<QDBusObjectPath>() == objectPath) {
            return i;
        }
    }
    return -1;
}

QVariant ProfileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return i18n("Profiles");
    }
    return QVariant();
}

bool ProfileModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(value)
    Q_UNUSED(role)

    QStandardItem *stdItem = itemFromIndex(index);
    QDBusMessage message;
    QDBusObjectPath parentObjPath = stdItem->data(ParentObjectPathRole).value<QDBusObjectPath>();
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             parentObjPath.path(),
                                             QLatin1String("org.freedesktop.ColorManager.Profile"),
                                             QLatin1String("MakeProfileDefault"));
    message << stdItem->data(ObjectPathRole);
    QDBusConnection::systemBus().send(message);

    // We return false since colord will emit a DeviceChanged signal telling us about this change
    return false;
}

Qt::ItemFlags ProfileModel::flags(const QModelIndex &index) const
{
    QStandardItem *stdItem = itemFromIndex(index);
    if (stdItem && stdItem->isCheckable() && stdItem->checkState() == Qt::Unchecked) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    }
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

#include "ProfileModel.moc"
