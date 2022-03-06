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

#include "Description.h"

#include "Profile.h"
#include "ProfileMetaData.h"

#include "CdInterface.h"
#include "CdDeviceInterface.h"
#include "CdProfileInterface.h"
#include "CdSensorInterface.h"

#include <QFileInfo>
#include <QStringBuilder>
#include <QDebug>
#include <QDateTime>
#include <QDebug>

#include <KLocalizedString>
#include <KToolInvocation>
#include <KMessageWidget>
#include <KFormat>

typedef QList<QDBusObjectPath> ObjectPathList;

Description::Description(QObject *parent)
    : QObject(parent)
{
}

void Description::setCdInterface(CdInterface *interface)
{
    // listen to colord for events
    connect(interface, &CdInterface::SensorAdded,
            this, &Description::sensorAddedUpdateCalibrateButton);
    connect(interface, &CdInterface::SensorRemoved,
            this, &Description::sensorRemovedUpdateCalibrateButton);

    auto async = interface->GetSensors();
    auto watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &Description::gotSensors);
}

void Description::setProfile(const QDBusObjectPath &objectPath, bool canRemoveProfile)
{
    m_currentProfile = objectPath;
    m_currentDeviceId.clear();

    CdProfileInterface profileInterface(QStringLiteral("org.freedesktop.ColorManager"),
                                        objectPath.path(),
                                        QDBusConnection::systemBus());
    if (!profileInterface.isValid()) {
        return;
    }

    const QString filename = profileInterface.filename();
    const bool hasVcgt = profileInterface.hasVcgt();
    const qlonglong created = profileInterface.created();

    Profile profile(filename);
    if (profile.loaded()) {
        // Set the profile type
        m_type = profile.kindString();

        // Set the colorspace
        m_colorspace = profile.colorspace();

        // Set the version
        m_version = profile.version();

        // Set the created time
        QDateTime createdDT = QDateTime::fromMSecsSinceEpoch(created);
        m_created = QLocale::system().toString(createdDT, QLocale::LongFormat);

        // Set the license
        m_license = profile.copyright();

        // Set the manufacturer
        m_deviceManufacturer = profile.manufacturer();

        // Set the Model
        m_deviceModel = profile.model();

        // Set the Display Correction
        m_dpCorrection = hasVcgt ? i18n("Yes") : i18n("None");

        // Set the file size
        m_filesize = KFormat().formatByteSize(profile.size());

        // Set the file name
        QFileInfo fileinfo(profile.filename());
        m_filename = fileinfo.fileName();

        QString temp;
        const uint temperature = profile.temperature();
        if (qAbs(temperature - 5000) < 10) {
            temp = QString::fromUtf8("%1K (D50)").arg(QString::number(temperature));
        } else if (qAbs(temperature - 6500) < 10) {
            temp = QString::fromUtf8("%1K (D65)").arg(QString::number(temperature));
        } else {
            temp = QString::fromUtf8("%1K").arg(QString::number(temperature));
        }
        m_whitepoint = temp;

        qDebug() << profile.description();
        qDebug() << profile.model();
        qDebug() << profile.manufacturer();
        qDebug() << profile.copyright();

        // Get named colors
        //        QMap<QString, QColor> namedColors = profile.getNamedColors();

        //        CdStringMap metadata = profileInterface.metadata();
    }
    qDebug() << profile.filename();
}

void Description::setDevice(const QDBusObjectPath &objectPath)
{
    CdDeviceInterface device(QStringLiteral("org.freedesktop.ColorManager"),
                             objectPath.path(),
                             QDBusConnection::systemBus());
    if (!device.isValid()) {
        return;
    }

    QString deviceTitle;
    m_currentDeviceId = device.deviceId();
    QString kind = device.kind();
    m_currentDeviceKind = kind;
    const QString model = device.model();
    const QString vendor = device.vendor();
    QString scope = device.scope();
    if (model.isEmpty() && vendor.isEmpty()) {
        m_deviceTitle = m_currentDeviceId;
    } else if (model.isEmpty()) {
        m_deviceTitle = vendor;
    } else if (vendor.isEmpty()) {
        m_deviceTitle = model;
    } else {
        m_deviceTitle = vendor % QLatin1String(" - ") % model;
    }

    if (kind == QLatin1String("printer")) {
        kind = i18nc("device type", "Printer");
    } else if (kind == QLatin1String("display")) {
        kind = i18nc("device type", "Display");
    } else if (kind == QLatin1String("webcam")) {
        kind = i18nc("device type", "Webcam");
    } else if (kind == QLatin1String("scanner")) {
        kind = i18nc("device type", "Scanner");
    } else {
        kind = i18nc("device type", "Unknown");
    }
    m_deviceKindTranslated = kind;
    m_deviceId = m_currentDeviceId;

    if (scope == QLatin1String("temp")) {
        scope = i18nc("device scope", "User session");
    } else if (scope == QLatin1String("disk")) {
        scope = i18nc("device scope", "Auto restore");
    } else if (scope == QLatin1String("normal")) {
        scope = i18nc("device scope", "System wide");
    } else {
        scope = i18nc("device scope", "Unknown");
    }
    m_deviceScope = scope;

    QString colorspace = device.colorspace();
    if (colorspace == QLatin1String("rgb")) {
        colorspace = i18nc("colorspace", "RGB");
    } else if (colorspace == QLatin1String("cmyk")) {
        colorspace = i18nc("colorspace", "CMYK");
    } else if (colorspace == QLatin1String("gray")) {
        colorspace = i18nc("colorspace", "Gray");
    }
    m_mode = colorspace;

    ObjectPathList profiles = device.profiles();

    QString profileTitle = i18n("This device has no profile assigned to it");
    if (!profiles.isEmpty()) {
        CdProfileInterface profile(QStringLiteral("org.freedesktop.ColorManager"),
                                   profiles.first().path(),
                                   QDBusConnection::systemBus());
        if (profile.isValid()) {
            profileTitle = profile.title();
            if (profileTitle.isEmpty()) {
                profileTitle = profile.profileId();
            }
        }
    }
    m_defaultProfileName = profileTitle;

    // Verify if the Calibrate button should be enabled or disabled
    m_calibrateButtonEnabled = calibrateEnabled(m_currentDeviceKind);
    Q_EMIT dataChanged();
    if (!m_isDevice) {
        m_isDevice = true;
        Q_EMIT isDeviceChanged();
    }
}

void Description::on_installSystemWideBt_clicked()
{
    CdProfileInterface profile(QStringLiteral("org.freedesktop.ColorManager"),
                               m_currentProfile.path(),
                               QDBusConnection::systemBus());
    profile.InstallSystemWide();
}

void Description::on_calibratePB_clicked()
{
    //    const QStringList args = {
    //        QLatin1String("--parent-window"),
    //        QString::number(winId()),
    //        QLatin1String("--device"),
    //        m_currentDeviceId
    //    };

    //    KToolInvocation::kdeinitExec(QLatin1String("gcm-calibrate"), args);
}

void Description::gotSensors(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<ObjectPathList> reply = *call;
    if (reply.isError()) {
        qWarning() << "Unexpected message" << reply.error().message();
    } else {
        const ObjectPathList sensors = reply.argumentAt<0>();
        for (const QDBusObjectPath &sensor : sensors) {
            // Add the sensors but don't update the Calibrate button
            sensorAdded(sensor, false);
        }

        // Update the calibrate button later
        calibrateEnabled(m_currentDeviceKind);
    }
}

void Description::sensorAdded(const QDBusObjectPath &sensorPath, bool updateCalibrateButton)
{
    if (!m_sensors.contains(sensorPath)) {
        m_sensors.append(sensorPath);
    }

    if (updateCalibrateButton) {
        calibrateEnabled(m_currentDeviceKind);
    }
}

void Description::sensorAddedUpdateCalibrateButton(const QDBusObjectPath &sensorPath)
{
    sensorAdded(sensorPath);
}

void Description::sensorRemoved(const QDBusObjectPath &sensorPath, bool updateCalibrateButton)
{
    m_sensors.removeOne(sensorPath);
    if (updateCalibrateButton) {
        calibrateEnabled(m_currentDeviceKind);
    }
}

void Description::sensorRemovedUpdateCalibrateButton(const QDBusObjectPath &sensorPath)
{
    sensorRemoved(sensorPath);
}

void Description::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(serviceName)
    if (newOwner.isEmpty() || oldOwner != newOwner) {
        // colord has quit or restarted
        m_sensors.clear();
    }
}

bool Description::calibrateEnabled(const QString &kind)
{
    QString toolTip;
    bool canCalibrate = false;
    toolTip = i18n("Create a color profile for the selected device");

    if (m_currentDeviceId.isEmpty()) {
        // No device was selected
        return false;
    }

    QFileInfo gcmCalibrate(QStandardPaths::findExecutable(QStringLiteral("gcm-calibrate")));
    if (!gcmCalibrate.isExecutable()) {
        // We don't have a calibration tool yet
        toolTip = i18n("You need Gnome Color Management installed in order to calibrate devices");
    } else if (kind == QLatin1String("display")) {
        if (m_sensors.isEmpty()) {
            toolTip = i18n("The measuring instrument is not detected. Please check it is turned on and correctly connected.");
        } else {
            canCalibrate = true;
        }
    } else if (kind == QLatin1String("camera") ||
               kind == QLatin1String("scanner") ||
               kind == QLatin1String("webcam")) {
        canCalibrate = true;
    } else if (kind == QLatin1String("printer")) {
        // Check if we have any sensor
        if (m_sensors.isEmpty()) {
            toolTip = i18n("The measuring instrument is not detected. Please check it is turned on and correctly connected.");
        } else {
            // Search for a suitable sensor
            for (const QDBusObjectPath &sensorPath : std::as_const(m_sensors)) {
                CdSensorInterface sensor(QStringLiteral("org.freedesktop.ColorManager"),
                                         sensorPath.path(),
                                         QDBusConnection::systemBus());
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
                toolTip = i18n("The measuring instrument does not support printer profiling.");
            }
        }
    } else {
        toolTip = i18n("The device type is not currently supported.");
    }

    if (canCalibrate) {
        m_calibrateButtonTooltip.clear();
    } else {
        m_calibrateButtonTooltip = toolTip;
    }

    Q_EMIT calibrateChanged();
    return canCalibrate;
}
