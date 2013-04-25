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

#include "CdInterface.h"
#include "CdProfileInterface.h"

#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusArgument>
#include <QStringBuilder>
#include <QFileInfo>

#include <KDebug>
#include <KLocale>
#include <KGlobal>
#include <KDateTime>
#include <KIcon>

typedef QList<QDBusObjectPath> ObjectPathList;

ProfileModel::ProfileModel(CdInterface *cdInterface, QObject *parent) :
    QStandardItemModel(parent)
{
    qDBusRegisterMetaType<ObjectPathList>();

    // listen to colord for events
    connect(cdInterface, SIGNAL(ProfileAdded(QDBusObjectPath)),
            this, SLOT(profileAdded(QDBusObjectPath)));
    connect(cdInterface, SIGNAL(ProfileRemoved(QDBusObjectPath)),
            this, SLOT(profileRemoved(QDBusObjectPath)));
    connect(cdInterface, SIGNAL(ProfileChanged(QDBusObjectPath)),
            this, SLOT(profileChanged(QDBusObjectPath)));

    // Ask for profiles
    QDBusPendingReply<ObjectPathList> async = cdInterface->GetProfiles();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(gotProfiles(QDBusPendingCallWatcher*)));
}

void ProfileModel::gotProfiles(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<ObjectPathList> reply = *call;
    if (reply.isError()) {
        kWarning() << "Unexpected message" << reply.error().message();
    } else {
        ObjectPathList profiles = reply.argumentAt<0>();
        foreach (const QDBusObjectPath &profile, profiles) {
            profileAdded(profile, false);
        }
        emit changed();
    }
    call->deleteLater();
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

    CdProfileInterface profile(QLatin1String("org.freedesktop.ColorManager"),
                               objectPath.path(),
                               QDBusConnection::systemBus());
    if (!profile.isValid()) {
        return;
    }

    // Verify if the profile has a filename
    QString filename = profile.filename();
    if (filename.isEmpty()) {
        return;
    }

    // Check if we can read the profile
    QFileInfo fileInfo(filename);
    if (!fileInfo.isReadable()) {
        return;
    }

    QString dataSource = getProfileDataSource(objectPath);
    QString profileId = profile.profileId();
    QString title = profile.title();
    QString kind = profile.kind();
    QString colorspace = profile.colorspace();
    qulonglong created = profile.created();

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

    // Sets the profile title
    if (title.isEmpty()) {
        title = profileId;
    } else {
        KDateTime createdDT;
        createdDT.setTime_t(created);
        title = Profile::profileWithSource(dataSource, title, createdDT);
    }
    item->setText(title);

    item->setData(qVariantFromValue(objectPath), ObjectPathRole);
    item->setData(qVariantFromValue(getSortChar(kind) + title), SortRole);
    item->setData(filename, FilenameRole);
    item->setData(kind, ProfileKindRole);

    if (!filename.isNull() && fileInfo.isWritable()) {
        // if we can write we can also remove the profile
        item->setData(true, CanRemoveProfileRole);
    }

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

void ProfileModel::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(serviceName)
    if (newOwner.isEmpty() || oldOwner != newOwner) {
        // colord has quit or restarted
        removeRows(0, rowCount());
        emit changed();
    }
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

QString ProfileModel::getProfileDataSource(const QDBusObjectPath &objectPath)
{
    QString dataSource;
    CdProfileInterface profile(QLatin1String("org.freedesktop.ColorManager"),
                               objectPath.path(),
                               QDBusConnection::systemBus());

    CdStringMap metadata = profile.metadata();
    if (metadata.contains(QLatin1String("DATA_source"))) {
        dataSource = metadata[QLatin1String("DATA_source")];
    }

    return dataSource;
}

QVariant ProfileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return i18n("Profiles");
    }
    return QVariant();
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
