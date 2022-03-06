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

#ifndef DESCRIPTION_H
#define DESCRIPTION_H

#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
class CdInterface;
class Description : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isDevice MEMBER m_isDevice NOTIFY isDeviceChanged)
    Q_PROPERTY(QString type MEMBER m_type NOTIFY dataChanged)
    Q_PROPERTY(QString colorspace MEMBER m_colorspace NOTIFY dataChanged)
    Q_PROPERTY(QString version MEMBER m_version NOTIFY dataChanged)
    Q_PROPERTY(QString created MEMBER m_created NOTIFY dataChanged)
    Q_PROPERTY(QString license MEMBER m_license NOTIFY dataChanged)
    Q_PROPERTY(QString deviceManufacturer MEMBER m_deviceManufacturer NOTIFY dataChanged)
    Q_PROPERTY(QString deviceModel MEMBER m_deviceModel NOTIFY dataChanged)
    Q_PROPERTY(QString dpCorrection MEMBER m_dpCorrection NOTIFY dataChanged)
    Q_PROPERTY(QString filesize MEMBER m_filesize NOTIFY dataChanged)
    Q_PROPERTY(QString filename MEMBER m_filename NOTIFY dataChanged)
    Q_PROPERTY(QString whitepoint MEMBER m_whitepoint NOTIFY dataChanged)
    Q_PROPERTY(QString deviceTitle MEMBER m_deviceTitle NOTIFY dataChanged)
    Q_PROPERTY(QString deviceId MEMBER m_deviceId NOTIFY dataChanged)
    Q_PROPERTY(QString deviceScope MEMBER m_deviceScope NOTIFY dataChanged)
    Q_PROPERTY(QString mode MEMBER m_mode NOTIFY dataChanged)
    Q_PROPERTY(QString defaultProfileName MEMBER m_defaultProfileName NOTIFY dataChanged)
    Q_PROPERTY(QString deviceKindTranslated MEMBER m_deviceKindTranslated NOTIFY dataChanged)
    Q_PROPERTY(QString calibrateButtonTooltip MEMBER m_calibrateButtonTooltip NOTIFY calibrateChanged)
    Q_PROPERTY(bool calibrateButtonEnabled MEMBER m_calibrateButtonEnabled NOTIFY calibrateChanged)

public:
    explicit Description(QObject *parent = nullptr);

    void setCdInterface(CdInterface *interface);
    void setProfile(const QDBusObjectPath &objectPath, bool canRemoveProfile);
    void setDevice(const QDBusObjectPath &objectPath);

Q_SIGNALS:
    void isDeviceChanged();
    void dataChanged();
    void calibrateChanged();

public Q_SLOTS:
    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);

private Q_SLOTS:
    void on_installSystemWideBt_clicked();
    void on_calibratePB_clicked();

    void gotSensors(QDBusPendingCallWatcher *call);
    void sensorAdded(const QDBusObjectPath &sensorPath, bool updateCalibrateButton = true);
    void sensorAddedUpdateCalibrateButton(const QDBusObjectPath &sensorPath);
    void sensorRemoved(const QDBusObjectPath &sensorPath, bool updateCalibrateButton = true);
    void sensorRemovedUpdateCalibrateButton(const QDBusObjectPath &sensorPath);

private:
    bool calibrateEnabled(const QString &kind);

    QDBusObjectPath m_currentProfile;
    QList<QDBusObjectPath> m_sensors;

    QString m_type;
    QString m_colorspace;
    QString m_version;
    QString m_created;
    QString m_license;
    QString m_deviceManufacturer;
    QString m_deviceModel;
    QString m_dpCorrection;
    QString m_filesize;
    QString m_filename;
    QString m_whitepoint;

    QString m_deviceTitle;
    QString m_deviceId;
    QString m_deviceScope;
    QString m_mode;
    QString m_defaultProfileName;
    QString m_currentDeviceId;
    QString m_currentDeviceKind;
    QString m_deviceKindTranslated;
    QString m_calibrateButtonTooltip;
    bool m_calibrateButtonEnabled = true;

    bool m_isDevice = true;
};

#endif // DESCRIPTION_H
