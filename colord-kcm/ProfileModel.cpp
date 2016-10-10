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

#include "ProfileModel.h"
#include "Profile.h"

#include "CdInterface.h"
#include "CdProfileInterface.h"

#include <QDBusMetaType>
#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusArgument>
#include <QStringBuilder>
#include <QFileInfo>
#include <QDebug>
#include <QIcon>
#include <QDateTime>

#include <KLocalizedString>

typedef QList<QDBusObjectPath> ObjectPathList;

ProfileModel::ProfileModel(CdInterface *cdInterface, QObject *parent) :
    QStandardItemModel(parent)
{
    qDBusRegisterMetaType<ObjectPathList>();
    qDBusRegisterMetaType<CdStringMap>();

    // listen to colord for events
    connect(cdInterface, &CdInterface::ProfileAdded,
            this, &ProfileModel::profileAddedEmit);
    connect(cdInterface, &CdInterface::ProfileRemoved,
            this, &ProfileModel::profileRemoved);
    connect(cdInterface, &CdInterface::ProfileChanged,
            this, &ProfileModel::profileChanged);

    // Ask for profiles
    auto async = cdInterface->GetProfiles();
    auto watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &ProfileModel::gotProfiles);
}

void ProfileModel::gotProfiles(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<ObjectPathList> reply = *call;
    if (reply.isError()) {
        qWarning() << "Unexpected message" << reply.error().message();
    } else {
        const ObjectPathList profiles = reply.argumentAt<0>();
        for (const QDBusObjectPath &profile : profiles) {
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
        qWarning() << "Profile not found" << objectPath.path();
        return;
    }

    // TODO what should we do when the profile changes?
}

void ProfileModel::profileAdded(const QDBusObjectPath &objectPath, bool emitChanged)
{
    if (findItem(objectPath) != -1) {
        qWarning() << "Profile is already on the list" << objectPath.path();
        return;
    }

    CdProfileInterface profile(QStringLiteral("org.freedesktop.ColorManager"),
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

    const QString dataSource = getProfileDataSource(profile.metadata());
    const QString profileId = profile.profileId();
    QString title = profile.title();
    const QString kind = profile.kind();
    const QString colorspace = profile.colorspace();
    const qlonglong created = profile.created();

    QStandardItem *item = new QStandardItem;

    // Choose a nice icon
    if (kind == QLatin1String("display-device")) {
        item->setIcon(QIcon::fromTheme(QStringLiteral("video-display")));
    } else if (kind == QLatin1String("input-device")) {
        item->setIcon(QIcon::fromTheme(QStringLiteral("scanner")));
    } else if (kind == QLatin1String("output-device")) {
        if (colorspace == QLatin1String("gray")) {
            item->setIcon(QIcon::fromTheme(QStringLiteral("printer-laser")));
        } else {
            item->setIcon(QIcon::fromTheme(QStringLiteral("printer")));
        }
    } else if (kind == QLatin1String("colorspace-conversion")) {
        item->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    } else if (kind == QLatin1String("abstract")) {
        item->setIcon(QIcon::fromTheme(QStringLiteral("insert-link")));
    } else if (kind == QLatin1String("named-color")) {
        item->setIcon(QIcon::fromTheme(QStringLiteral("view-preview")));
    } else {
        item->setIcon(QIcon::fromTheme(QStringLiteral("image-missing")));
    }

    // Sets the profile title
    if (title.isEmpty()) {
        title = profileId;
    } else {
        QDateTime createdDT;
        createdDT.setTime_t(created);
        title = Profile::profileWithSource(dataSource, title, createdDT);
    }
    item->setText(title);

    item->setData(qVariantFromValue(objectPath), ObjectPathRole);
    item->setData(QString(getSortChar(kind) + title), SortRole);
    item->setData(filename, FilenameRole);
    item->setData(kind, ProfileKindRole);

    bool canRemoveProfile = false;
    if (!filename.isNull() &&
            fileInfo.isWritable() &&
            dataSource != QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_EDID)) {
        // if we can write we can also remove the profile
        canRemoveProfile = true;
    }
    item->setData(canRemoveProfile, CanRemoveProfileRole);

    appendRow(item);

    if (emitChanged) {
        emit changed();
    }
}

void ProfileModel::profileAddedEmit(const QDBusObjectPath &objectPath)
{
    profileAdded(objectPath);
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

QString ProfileModel::getProfileDataSource(const CdStringMap &metadata)
{
    QString dataSource;
    auto it = metadata.constFind(QStringLiteral("DATA_source"));
    if (it != metadata.constEnd()) {
        dataSource = it.value();
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
