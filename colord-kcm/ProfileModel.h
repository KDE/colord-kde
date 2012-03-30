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

#ifndef PROFILE_MODEL_H
#define PROFILE_MODEL_H

#include <QStandardItemModel>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusMessage>

typedef QMap<QString, QString>  StringStringMap;

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
        ProfileKindRole
    } ProfileRoles;
    explicit ProfileModel(QObject *parent = 0);

    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    // Returns a char to help the sort model
    static QChar getSortChar(const QString &kind);
    static QString getProfileDataSource(const QDBusObjectPath &objectPath);

signals:
    void changed();

private slots:
    void gotProfiles(const QDBusMessage &message);
    void profileChanged(const QDBusObjectPath &objectPath);
    void profileAdded(const QDBusObjectPath &objectPath, bool emitChanged = true);
    void profileRemoved(const QDBusObjectPath &objectPath);
    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);

private:
    int findItem(const QDBusObjectPath &objectPath);
};

#endif // PROFILE_MODEL_H
