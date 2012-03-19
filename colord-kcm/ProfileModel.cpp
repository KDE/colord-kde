/***************************************************************************
 *   Copyright (C) 2012 by Daniel Nicoletti <dantti12@gmail.com>           *
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
#include "Profile.h"

#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusArgument>
#include <QStringBuilder>
#include <QFileInfo>

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
    QDBusConnection::systemBus().callWithCallback(message, this, SLOT(gotProfiles(QDBusMessage)));
}

void ProfileModel::gotProfiles(const QDBusMessage &message)
{
    if (message.type() == QDBusMessage::ReplyMessage && message.arguments().size() == 1) {
        QDBusArgument argument = message.arguments().first().value<QDBusArgument>();
        ObjectPathList paths = qdbus_cast<ObjectPathList>(argument);
        foreach (const QDBusObjectPath &path, paths) {
            profileAdded(path, false);
        }
        emit changed();
    } else {
        kWarning() << "Unexpected message" << message;
    }
}

void ProfileModel::profileChanged(const QDBusObjectPath &objectPath)
{
    int row = findItem(objectPath);
    if (row == -1) {
        kWarning() << "Profile not found" << objectPath.path();
        return;
    }

    // TODO what should we do when the profile changes?
}

void ProfileModel::profileAdded(const QDBusObjectPath &objectPath, bool emitChanged)
{
    if (findItem(objectPath) != -1) {
        kWarning() << "Profile is already on the list" << objectPath.path();
        return;
    }

    QDBusInterface *profileInterface;
    profileInterface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                         objectPath.path(),
                                         QLatin1String("org.freedesktop.ColorManager.Profile"),
                                         QDBusConnection::systemBus(),
                                         this);
    if (!profileInterface->isValid()) {
        profileInterface->deleteLater();
        return;
    }

    // Verify if the profile has a filename
    QString filename = profileInterface->property("Filename").toString();
    if (filename.isEmpty()) {
        profileInterface->deleteLater();
        return;
    }

    // Check if we can read the profile
    QFileInfo fileInfo(filename);
    if (!fileInfo.isReadable()) {
        profileInterface->deleteLater();
        return;
    }

    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             objectPath.path(),
                                             QLatin1String("org.freedesktop.DBus.Properties"),
                                             QLatin1String("Get"));
    message << QString("org.freedesktop.ColorManager.Profile"); // Interface
    message << QString("Metadata"); // Propertie Name
    QDBusReply<QVariant> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);
    QString dataSource;
    if (reply.isValid()) {
        QDBusArgument argument = reply.value().value<QDBusArgument>();
        StringStringMap metadata = qdbus_cast<StringStringMap>(argument);
        StringStringMap::const_iterator i = metadata.constBegin();
        while (i != metadata.constEnd()) {
            if (i.key() == QLatin1String("DATA_source")) {
                // Found DATA_source
                dataSource = i.value();
                break;
            }
            ++i;
        }
    }

    QString profileId = profileInterface->property("ProfileId").toString();
    QString title = profileInterface->property("Title").toString();
    QString kind = profileInterface->property("Kind").toString();
    profileInterface->deleteLater();

    QString colorspace = profileInterface->property("Colorspace").toString();

    QStandardItem *item = new QStandardItem;

    // Choose a nice icon
    if (kind == QLatin1String("display-device")) {
        item->setIcon(KIcon(QLatin1String("video-display")));
    } else if (kind == QLatin1String("input-device")) {
        item->setIcon(KIcon(QLatin1String("scanner")));
    } else if (kind == QLatin1String("output-device")) {
        if (colorspace == QLatin1String("gray")) {
            item->setIcon(KIcon(QLatin1String("printer-laser")));
        } else {
            item->setIcon(KIcon(QLatin1String("printer")));
        }
    } else if (kind == QLatin1String("colorspace-conversion")) {
        item->setIcon(KIcon(QLatin1String("view-refresh")));
    } else if (kind == QLatin1String("abstract")) {
        item->setIcon(KIcon(QLatin1String("insert-link")));
    } else if (kind == QLatin1String("named-color")) {
            item->setIcon(KIcon(QLatin1String("view-preview")));
    } else {
        item->setIcon(KIcon(QLatin1String("image-missing")));
    }

    if (title.isEmpty()) {
        title = profileId;
    }
    item->setText(title);

    item->setData(qVariantFromValue(objectPath), ObjectPathRole);
    item->setData(qVariantFromValue(getSortChar(kind) + title), SortRole);
    item->setData(filename, FilenameRole);
    item->setData(kind, ProfileKindRole);
    item->setData(Profile::profileWithSource(dataSource, title), ProfileDisplayNameSourceRole);

    appendRow(item);

    if (emitChanged) {
        emit changed();
    }
}

void ProfileModel::profileRemoved(const QDBusObjectPath &objectPath)
{
    int row = findItem(objectPath);
    if (row != -1) {
        removeRow(row);
    }

    emit changed();
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

QChar ProfileModel::getSortChar(const QString &kind)
{
    if (kind == QLatin1String("display-device")) {
        return QLatin1Char('1');
    }
    if (kind == QLatin1String("input-device")) {
        return QLatin1Char('2');
    }
    if (kind == QLatin1String("output-device")) {
        return QLatin1Char('3');
    }
    return QLatin1Char('4');
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
