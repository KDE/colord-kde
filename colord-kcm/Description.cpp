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

#include "Description.h"
#include "ui_Description.h"

#include "Profile.h"
#include "ProfileNamedColors.h"
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

#define TAB_INFORMATION  1
#define TAB_CIE_1931     2
#define TAB_TRC          3
#define TAB_VCGT         4
#define TAB_FROM_SRGB    5
#define TAB_TO_SRGB      6
#define TAB_NAMED_COLORS 7
#define TAB_METADATA     8

typedef QList<QDBusObjectPath> ObjectPathList;

Description::Description(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Description)
{
    ui->setupUi(this);
    ui->msgWidget->setMessageType(KMessageWidget::Warning);
    ui->msgWidget->setWordWrap(true);
    ui->msgWidget->setCloseButtonVisible(false);
    ui->msgWidget->hide();

    m_namedColors = new ProfileNamedColors;
    m_metadata = new ProfileMetaData;
}

Description::~Description()
{
    delete m_namedColors;
    delete m_metadata;

    delete ui;
}

int Description::innerHeight() const
{
    return ui->tabWidget->currentWidget()->height();
}

void Description::setCdInterface(CdInterface *interface)
{
    // listen to colord for events
    connect(interface, SIGNAL(SensorAdded(QDBusObjectPath)),
            this, SLOT(sensorAdded(QDBusObjectPath)));
    connect(interface, SIGNAL(SensorRemoved(QDBusObjectPath)),
            this, SLOT(sensorRemoved(QDBusObjectPath)));

    auto async = interface->GetSensors();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(gotSensors(QDBusPendingCallWatcher*)));
}

void Description::setProfile(const QDBusObjectPath &objectPath, bool canRemoveProfile)
{
    m_currentProfile = objectPath;
    m_currentDeviceId.clear();

    ui->stackedWidget->setCurrentIndex(0);
    CdProfileInterface profileInterface(QLatin1String("org.freedesktop.ColorManager"),
                                        objectPath.path(),
                                        QDBusConnection::systemBus());
    if (!profileInterface.isValid()) {
        return;
    }

    const QString filename = profileInterface.filename();
    const bool hasVcgt = profileInterface.hasVcgt();
    const qlonglong created = profileInterface.created();

    ui->installSystemWideBt->setEnabled(canRemoveProfile);
    Profile profile(filename);
    if (profile.loaded()) {
        // Set the profile type
        ui->typeL->setText(profile.kindString());

        // Set the colorspace
        ui->colorspaceL->setText(profile.colorspace());

        // Set the version
        ui->versionL->setText(profile.version());

        // Set the created time
        QDateTime createdDT;
        createdDT.setTime_t(created);
        ui->createdL->setText(QLocale().toString(createdDT, QLocale::LongFormat));

        // Set the license
        ui->licenseL->setText(profile.copyright());
        ui->licenseL->setVisible(!profile.copyright().isEmpty());
        ui->licenseLabel->setVisible(!profile.copyright().isEmpty());

        // Set the manufacturer
        ui->deviceMakeL->setText(profile.manufacturer());
        ui->deviceMakeL->setVisible(!profile.manufacturer().isEmpty());
        ui->makeLabel->setVisible(!profile.manufacturer().isEmpty());

        // Set the Model
        ui->deviceModelL->setText(profile.model());
        ui->deviceModelL->setVisible(!profile.model().isEmpty());
        ui->modelLabel->setVisible(!profile.model().isEmpty());

        // Set the Display Correction
        ui->dpCorrectionL->setText(hasVcgt ? i18n("Yes") : i18n("None"));

        // Set the file size
        ui->filesizeL->setText(KFormat().formatByteSize(profile.size()));

        // Set the file name
        QFileInfo fileinfo(profile.filename());
        ui->filenameL->setText(fileinfo.fileName());

        QString temp;
        const uint temperature = profile.temperature();
        if (qAbs(temperature - 5000) < 10) {
            temp = QString::fromUtf8("%1K (D50)").arg(QString::number(temperature));
        } else if (qAbs(temperature - 6500) < 10) {
            temp = QString::fromUtf8("%1K (D65)").arg(QString::number(temperature));
        } else {
            temp = QString::fromUtf8("%1K").arg(QString::number(temperature));
        }
        ui->whitepointL->setText(temp);

        qDebug() << profile.description();
        qDebug() << profile.model();
        qDebug() << profile.manufacturer();
        qDebug() << profile.copyright();

        // Get named colors
        QMap<QString, QColor> namedColors = profile.getNamedColors();
        if (!namedColors.isEmpty()) {
            m_namedColors->setNamedColors(namedColors);
            insertTab(TAB_NAMED_COLORS, m_namedColors, i18n("Named Colors"));
        } else {
            removeTab(m_namedColors);
        }

        CdStringMap metadata = profileInterface.metadata();
        if (!metadata.isEmpty()) {
            m_metadata->setMetadata(metadata);
            insertTab(TAB_METADATA, m_metadata, i18n("Metadata"));
        } else {
            removeTab(m_metadata);
        }
    }
    qDebug() << profile.filename();
}

void Description::setDevice(const QDBusObjectPath &objectPath)
{
    while (ui->tabWidget->count() > 1) {
        ui->tabWidget->removeTab(1);
    }

    ui->stackedWidget->setCurrentIndex(1);

    CdDeviceInterface device(QLatin1String("org.freedesktop.ColorManager"),
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
        deviceTitle = m_currentDeviceId;
    } else if (model.isEmpty()) {
        deviceTitle = vendor;
    } else if (vendor.isEmpty()) {
        deviceTitle = model;
    } else {
        deviceTitle = vendor % QLatin1String(" - ") % model;
    }
    ui->ktitlewidget->setText(deviceTitle);

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
    ui->ktitlewidget->setComment(kind);

    ui->deviceIdL->setText(m_currentDeviceId);

    if (scope == QLatin1String("temp")) {
        scope = i18nc("device scope", "User session");
    } else if (scope == QLatin1String("disk")) {
        scope = i18nc("device scope", "Auto restore");
    } else if (scope == QLatin1String("normal")) {
        scope = i18nc("device scope", "System wide");
    } else {
        scope = i18nc("device scope", "Unknown");
    }
    ui->deviceScopeL->setText(scope);

    QString colorspace = device.colorspace();
    if (colorspace == QLatin1String("rgb")) {
        colorspace = i18nc("colorspace", "RGB");
    } else if (colorspace == QLatin1String("cmyk")) {
        colorspace = i18nc("colorspace", "CMYK");
    } else if (colorspace == QLatin1String("gray")) {
        colorspace = i18nc("colorspace", "Gray");
    }
    ui->modeL->setText(colorspace);

    ObjectPathList profiles = device.profiles();

    QString profileTitle = i18n("This device has no profile assigned to it");
    if (!profiles.isEmpty()) {
        CdProfileInterface profile(QLatin1String("org.freedesktop.ColorManager"),
                                   profiles.first().path(),
                                   QDBusConnection::systemBus());
        if (profile.isValid()) {
            profileTitle = profile.title();
            if (profileTitle.isEmpty()) {
                profileTitle = profile.profileId();
            }
        }
    }
    ui->defaultProfileName->setText(profileTitle);

    // Verify if the Calibrate button should be enabled or disabled
    ui->calibratePB->setEnabled(calibrateEnabled(m_currentDeviceKind));
}

void Description::on_installSystemWideBt_clicked()
{
    CdProfileInterface profile(QLatin1String("org.freedesktop.ColorManager"),
                               m_currentProfile.path(),
                               QDBusConnection::systemBus());
    profile.InstallSystemWide();
}

void Description::on_calibratePB_clicked()
{
    QStringList args;
    args << QLatin1String("--parent-window");
    args << QString::number(winId());
    args << QLatin1String("--device");
    args << m_currentDeviceId;

    KToolInvocation::kdeinitExec(QLatin1String("gcm-calibrate"), args);
}

void Description::gotSensors(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<ObjectPathList> reply = *call;
    if (reply.isError()) {
        qWarning() << "Unexpected message" << reply.error().message();
    } else {
        ObjectPathList sensors = reply.argumentAt<0>();
        foreach (const QDBusObjectPath &sensor, sensors) {
            // Add the sensors but don't update the Calibrate button
            sensorAdded(sensor, false);
        }

        // Update the calibrate button later
        ui->calibratePB->setEnabled(calibrateEnabled(m_currentDeviceKind));
    }
}

void Description::sensorAdded(const QDBusObjectPath &sensorPath, bool updateCalibrateButton)
{
    if (!m_sensors.contains(sensorPath)) {
        m_sensors.append(sensorPath);
    }

    if (updateCalibrateButton) {
        ui->calibratePB->setEnabled(calibrateEnabled(m_currentDeviceKind));
    }
}

void Description::sensorRemoved(const QDBusObjectPath &sensorPath, bool updateCalibrateButton)
{
    m_sensors.removeOne(sensorPath);
    if (updateCalibrateButton) {
        ui->calibratePB->setEnabled(calibrateEnabled(m_currentDeviceKind));
    }
}

void Description::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(serviceName)
    if (newOwner.isEmpty() || oldOwner != newOwner) {
        // colord has quit or restarted
        m_sensors.clear();
    }
}

void Description::insertTab(int index, QWidget *widget, const QString &label)
{
    int pos = ui->tabWidget->indexOf(widget);
    if (pos == -1) {
        // if the widget was not found set the desired ORDER
        widget->setProperty("ORDER", index);
        pos = index;
        for (int i = 1; i < ui->tabWidget->count(); ++i) {
            QWidget *widget = ui->tabWidget->widget(i);
            // Check if the tab widget in greater than our desired position
            if (widget->property("ORDER").toInt() > index) {
                // if so make sure the new widget is inserted in the
                // same position of this widget, pushing it further
                pos = i;
                break;
            }
        }
        ui->tabWidget->insertTab(pos, widget, label);
    }
}

void Description::removeTab(QWidget *widget)
{
    int pos = ui->tabWidget->indexOf(widget);
    if (pos != -1) {
        ui->tabWidget->removeTab(pos);
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

    QFileInfo gcmCalibrate(QStandardPaths::findExecutable("gcm-calibrate"));
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
            foreach (const QDBusObjectPath &sensorPath, m_sensors) {
                CdSensorInterface sensor(QLatin1String("org.freedesktop.ColorManager"),
                                         sensorPath.path(),
                                         QDBusConnection::systemBus());
                if (!sensor.isValid()) {
                    continue;
                }

                QStringList capabilities = sensor.capabilities();
                if (capabilities.contains(QLatin1String("printer"))) {
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
        ui->calibratePB->setToolTip(toolTip);
        ui->msgWidget->hide();
    } else {
        ui->msgWidget->setText(toolTip);
        ui->msgWidget->show();
    }

    return canCalibrate;
}
