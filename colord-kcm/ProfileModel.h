/***************************************************************************
 *   Copyright (C) 2012-2016 by Daniel Nicoletti <dantti12@gmail.com>      *
 *   2022 by Han Young <hanyoung@protonmail.com>                           *
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

#pragma once

#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QStandardItemModel>

#include "dbus-types.h"

class CdInterface;
class ProfileModel : public QStandardItemModel
{
    Q_OBJECT
public:
    typedef enum {
        ObjectPathRole = Qt::UserRole + 1,
        ParentObjectPathRole,
        IsDeviceRole,
        SortRole,
        FilenameRole,
        ColorspaceRole,
        ProfileKindRole,
        CanRemoveProfileRole
    } ProfileRoles;
    explicit ProfileModel(CdInterface *cdInterface, QObject *parent = nullptr);

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Returns a char to help the sort model
    static QChar getSortChar(const QString &kind);
    static QString getProfileDataSource(const CdStringMap &metadata);
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);

Q_SIGNALS:
    void changed(); // not used actually

private Q_SLOTS:
    void gotProfiles(QDBusPendingCallWatcher *call);
    void profileChanged(const QDBusObjectPath &objectPath);
    void profileAdded(const QDBusObjectPath &objectPath, bool emitChanged = true);
    void profileAddedEmit(const QDBusObjectPath &objectPath);
    void profileRemoved(const QDBusObjectPath &objectPath);

private:
    int findItem(const QDBusObjectPath &objectPath);
};
