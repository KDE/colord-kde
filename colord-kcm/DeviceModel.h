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

#ifndef DEVICE_MODEL_H
#define DEVICE_MODEL_H

#include <QStandardItemModel>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusPendingCallWatcher>

typedef QList<QDBusObjectPath> ObjectPathList;

class CdInterface;
class DeviceModel : public QStandardItemModel
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
    } DeviceRoles;
    explicit DeviceModel(CdInterface *cdInterface, QObject *parent = 0);

    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;

public slots:
    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);

signals:
    void changed();

private slots:
    void gotDevices(QDBusPendingCallWatcher *call);
    void deviceChanged(const QDBusObjectPath &objectPath);
    void deviceAdded(const QDBusObjectPath &objectPath, bool emitChanged = true);
    void deviceRemoved(const QDBusObjectPath &objectPath);

private:
    QStandardItem* createProfileItem(const QDBusObjectPath &objectPath,
                                     const QDBusObjectPath &parentObjectPath,
                                     bool checked);
    QStandardItem* findProfile(QStandardItem *parent, const QDBusObjectPath &objectPath);
    void removeProfilesNotInList(QStandardItem *parent, const ObjectPathList &profiles);
    int findItem(const QDBusObjectPath &objectPath);

    CdInterface*m_cdInterface;
};

#endif // DEVICE_MODEL_H
