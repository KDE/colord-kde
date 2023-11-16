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

#include "DeviceDescription.h"

#include "CdDeviceInterface.h"
#include "CdInterface.h"
#include "CdProfileInterface.h"
#include "CdSensorInterface.h"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QStringBuilder>

#include <KFormat>
#include <KLocalizedString>
#include <KMessageWidget>

typedef QList<QDBusObjectPath> ObjectPathList;

DeviceDescription::DeviceDescription(QObject *parent)
    : QObject(parent)
{
}

void DeviceDescription::setCdInterface(CdInterface *interface)
{
    // listen to colord for events
    connect(interface, &CdInterface::SensorAdded, this, &DeviceDescription::sensorAddedUpdateCalibrateButton);
    connect(interface, &CdInterface::SensorRemoved, this, &DeviceDescription::sensorRemovedUpdateCalibrateButton);

    auto async = interface->GetSensors();
    auto watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &DeviceDescription::gotSensors);
}

void DeviceDescription::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(serviceName)
    if (newOwner.isEmpty() || oldOwner != newOwner) {
        // colord has quit or restarted
        m_sensors.clear();
    }
}

void DeviceDescription::gotSensors(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<ObjectPathList> reply = *call;
    if (reply.isError()) {
        qWarning() << "Unexpected message" << reply.error().message();
    } else {
        const ObjectPathList sensors = reply.argumentAt<0>();
        for (const QDBusObjectPath &sensor : sensors) {
            // Add the sensors but don't update the Calibrate message
            sensorAdded(sensor, false);
        }

        // Update the calibrate message later
        generateCalibrateMessage(m_currentDeviceKind);
    }
}

void DeviceDescription::sensorAdded(const QDBusObjectPath &sensorPath, bool updateCalibrateMessage)
{
    if (!m_sensors.contains(sensorPath)) {
        m_sensors.append(sensorPath);
    }

    if (updateCalibrateMessage) {
        generateCalibrateMessage(m_currentDeviceKind);
    }
}

void DeviceDescription::sensorAddedUpdateCalibrateButton(const QDBusObjectPath &sensorPath)
{
    sensorAdded(sensorPath);
}

void DeviceDescription::sensorRemoved(const QDBusObjectPath &sensorPath, bool updateCalibrateMessage)
{
    m_sensors.removeOne(sensorPath);
    if (updateCalibrateMessage) {
        generateCalibrateMessage(m_currentDeviceKind);
    }
}

void DeviceDescription::sensorRemovedUpdateCalibrateButton(const QDBusObjectPath &sensorPath)
{
    sensorRemoved(sensorPath);
}

void DeviceDescription::setDevice(const QDBusObjectPath &objectPath)
{
    CdDeviceInterface device(QStringLiteral("org.freedesktop.ColorManager"), objectPath.path(), QDBusConnection::systemBus());
    if (!device.isValid()) {
        return;
    }

    m_currentDeviceID = device.deviceId();
    const QString model = device.model();
    const QString vendor = device.vendor();
    if (model.isEmpty() && vendor.isEmpty()) {
        m_deviceTitle = m_currentDeviceID;
    } else if (model.isEmpty()) {
        m_deviceTitle = vendor;
    } else if (vendor.isEmpty()) {
        m_deviceTitle = model;
    } else {
        m_deviceTitle = vendor % QLatin1String(" - ") % model;
    }

    const QString kind = device.kind();
    if (kind == QLatin1String("printer")) {
        m_currentDeviceKind = i18nc("device type", "Printer");
    } else if (kind == QLatin1String("display")) {
        m_currentDeviceKind = i18nc("device type", "Display");
    } else if (kind == QLatin1String("webcam")) {
        m_currentDeviceKind = i18nc("device type", "Webcam");
    } else if (kind == QLatin1String("scanner")) {
        m_currentDeviceKind = i18nc("device type", "Scanner");
    } else {
        m_currentDeviceKind = i18nc("device type", "Unknown");
    }

    const QString scope = device.scope();
    if (scope == QLatin1String("temp")) {
        m_deviceScope = i18nc("device scope", "User session");
    } else if (scope == QLatin1String("disk")) {
        m_deviceScope = i18nc("device scope", "Auto restore");
    } else if (scope == QLatin1String("normal")) {
        m_deviceScope = i18nc("device scope", "System wide");
    } else {
        m_deviceScope = i18nc("device scope", "Unknown");
    }

    const QString colorspace = device.colorspace();
    if (colorspace == QLatin1String("rgb")) {
        m_colorSpace = i18nc("colorspace", "RGB");
    } else if (colorspace == QLatin1String("cmyk")) {
        m_colorSpace = i18nc("colorspace", "CMYK");
    } else if (colorspace == QLatin1String("gray")) {
        m_colorSpace = i18nc("colorspace", "Gray");
    }

    ObjectPathList profiles = device.profiles();

    QString profileTitle = i18n("This device has no profile assigned to it");
    if (!profiles.isEmpty()) {
        CdProfileInterface profile(QStringLiteral("org.freedesktop.ColorManager"), profiles.first().path(), QDBusConnection::systemBus());
        if (profile.isValid()) {
            profileTitle = profile.title();
            if (profileTitle.isEmpty()) {
                profileTitle = profile.profileId();
            }
        }
    }
    m_currentProfileTitle = profileTitle;

    generateCalibrateMessage(kind);

    Q_EMIT dataChanged();
}

void DeviceDescription::generateCalibrateMessage(const QString &kind)
{
    m_calibrateMsg = i18n("You can use 'displaycal' to calibrate this device");

    if (m_currentDeviceID.isEmpty()) {
        // No device was selected
        return;
    }

    if (kind == QLatin1String("display")) {
        if (m_sensors.isEmpty()) {
            m_calibrateMsg = i18n("The measuring instrument for calibrating is not detected. Please check it is turned on and correctly connected.");
        }
    } else if (kind == QLatin1String("printer")) {
        // Check if we have any sensor
        if (m_sensors.isEmpty()) {
            m_calibrateMsg = i18n("The measuring instrument for calibrating is not detected. Please check it is turned on and correctly connected.");
        } else {
            bool canCalibrate = false;
            // Search for a suitable sensor
            for (const QDBusObjectPath &sensorPath : std::as_const(m_sensors)) {
                CdSensorInterface sensor(QStringLiteral("org.freedesktop.ColorManager"), sensorPath.path(), QDBusConnection::systemBus());
                if (!sensor.isValid()) {
                    continue;
                }

                QStringList capabilities = sensor.capabilities();
                if (capabilities.contains(QStringLiteral("printer"))) {
                    canCalibrate = true;
                    break;
                }
            }

            // If we did not find a suitable sensor, display a warning
            if (!canCalibrate) {
                m_calibrateMsg = i18n("The measuring instrument does not support printer profiling.");
            }
        }
    } else if (kind != QLatin1String("camera") && kind != QLatin1String("scanner") && kind != QLatin1String("webcam")) {
        m_calibrateMsg = i18n("The device type is not currently supported for calibrating.");
    }

    Q_EMIT calibrateMessageChanged();
}

#include "moc_DeviceDescription.cpp"
