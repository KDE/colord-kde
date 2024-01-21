/***************************************************************************
 *   Copyright (C) 2012 by Daniel Nicoletti                                *
 *   dantti12@gmail.com                                                    *
 *   2022 by Han Young <hanyoung@protonmail.com                            *
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

#include <KConfigGroup>
#include <KQuickConfigModule>
#include <QDBusObjectPath>

class DeviceModel;
class ProfileModel;
class CdInterface;
class DeviceDescription;
class ProfileDescription;
class QSortFilterProxyModel;
class AddProfileComboBoxItem;
class KCMColord : public KQuickConfigModule
{
    Q_OBJECT
    Q_PROPERTY(DeviceModel *deviceModel READ deviceModel CONSTANT)
    Q_PROPERTY(ProfileModel *profileModel READ profileModel CONSTANT)
    Q_PROPERTY(DeviceDescription *deviceDescription READ deviceDescription CONSTANT)
    Q_PROPERTY(ProfileDescription *profileDescription READ profileDescription CONSTANT)

public:
    explicit KCMColord(QObject *parent, const KPluginMetaData &data, const QVariantList &list = QVariantList());
    DeviceModel *deviceModel() const;
    ProfileModel *profileModel() const;
    DeviceDescription *deviceDescription() const;
    ProfileDescription *profileDescription() const;

    Q_INVOKABLE void makeProfileDefault(const QDBusObjectPath &profilePath, const QDBusObjectPath &devicePath);
    Q_INVOKABLE void assignProfileToDevice(const QDBusObjectPath &profile, const QDBusObjectPath &devicePath) const;
    Q_INVOKABLE bool canAddProfileForDevice(const QDBusObjectPath &device);

    // DO NOT RETAIN OR USE ANY OBJECT IN THE FIRST CALL AFTER SECOND CALL!!!
    // This method destroy any objects allocated in the previous call after
    // returning for the second call. This behavior is to prevent memory leak
    Q_INVOKABLE QList<QObject *> getComboBoxItemsForDevice(const QDBusObjectPath &device);
    Q_INVOKABLE void importProfile(const QUrl &filepath);
    Q_INVOKABLE void removeProfile(const QString &filename);

private:
    CdInterface *const m_cdInterface;
    DeviceModel *const m_deviceModel;
    ProfileModel *const m_profileModel;
    DeviceDescription *const m_deviceDescription;
    ProfileDescription *const m_profileDescription;
    QSortFilterProxyModel *const m_filterModel;
    QList<QObject *> m_comboBoxItemsToBeDestroyed;
};

class AddProfileComboBoxItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDBusObjectPath objectPath MEMBER m_objectPath NOTIFY dataChanged)
    Q_PROPERTY(QString profileName MEMBER m_profileName NOTIFY dataChanged)
public:
    AddProfileComboBoxItem(QObject *parent, const QDBusObjectPath &path, const QString &name)
        : QObject(parent)
        , m_objectPath(path)
        , m_profileName(name)
    {
    }

Q_SIGNALS:
    void dataChanged();

private:
    QDBusObjectPath m_objectPath;
    QString m_profileName;
};
