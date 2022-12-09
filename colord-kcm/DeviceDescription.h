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

#ifndef DESCRIPTION_H
#define DESCRIPTION_H

#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
class CdInterface;
class DeviceDescription : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString colorSpace MEMBER m_colorSpace NOTIFY dataChanged)
    Q_PROPERTY(QString deviceTitle MEMBER m_deviceTitle NOTIFY dataChanged)
    Q_PROPERTY(QString deviceID MEMBER m_currentDeviceID NOTIFY dataChanged)
    Q_PROPERTY(QString deviceScope MEMBER m_deviceScope NOTIFY dataChanged)
    Q_PROPERTY(QString deviceKind MEMBER m_currentDeviceKind NOTIFY dataChanged)
    Q_PROPERTY(QString currentProfileTitle MEMBER m_currentProfileTitle NOTIFY dataChanged)
    Q_PROPERTY(QString calibrateTipMessage MEMBER m_calibrateMsg NOTIFY calibrateMessageChanged)

public:
    explicit DeviceDescription(QObject *parent = nullptr);

    void setCdInterface(CdInterface *interface);
    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);

    Q_INVOKABLE void setDevice(const QDBusObjectPath &objectPath);

Q_SIGNALS:
    void isDeviceChanged();
    void dataChanged();
    void calibrateChanged();
    void calibrateMessageChanged();

private Q_SLOTS:
    void gotSensors(QDBusPendingCallWatcher *call);
    void sensorAddedUpdateCalibrateButton(const QDBusObjectPath &sensorPath);
    void sensorRemovedUpdateCalibrateButton(const QDBusObjectPath &sensorPath);

private:
    void generateCalibrateMessage(const QString &kind);
    void sensorAdded(const QDBusObjectPath &sensorPath, bool updateCalibrateMessage = true);
    void sensorRemoved(const QDBusObjectPath &sensorPath, bool updateCalibrateMessage = true);

    QDBusObjectPath m_currentProfile;

    QString m_deviceTitle;
    QString m_deviceScope;
    QString m_currentDeviceKind;
    QString m_currentDeviceID;
    QString m_colorSpace;
    QString m_currentProfileTitle;
    QString m_calibrateMsg;
    QList<QDBusObjectPath> m_sensors;
};

#endif // DESCRIPTION_H
