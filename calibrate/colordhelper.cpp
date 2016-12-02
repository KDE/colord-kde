/***************************************************************************
 *   Copyright (C) 2016 by Daniel Nicoletti <dantti12@gmail.com>           *
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
#include "colordhelper.h"

#include "CdHelperInterface.h"
#include "CdInterface.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(CALIBRATE, "calibrate")

static QString service = QStringLiteral("org.freedesktop.ColorHelper");
static QString servicePath = QStringLiteral("/");

typedef QList<QDBusObjectPath> ObjectPathList;

ColordHelper::ColordHelper(const QString &deviceId, QObject *parent) : QObject(parent)
  , m_deviceId(deviceId)
{
    m_displayInterface = new OrgFreedesktopColorHelperDisplayInterface(service,
                                                                       servicePath,
                                                                       QDBusConnection::sessionBus(),
                                                                       this);
    connect(m_displayInterface, &OrgFreedesktopColorHelperDisplayInterface::Finished,
            this, &ColordHelper::Finished);
    connect(m_displayInterface, &OrgFreedesktopColorHelperDisplayInterface::InteractionRequired,
            this, &ColordHelper::InteractionRequired);
    connect(m_displayInterface, &OrgFreedesktopColorHelperDisplayInterface::UpdateGamma,
            this, &ColordHelper::UpdateGamma);
    connect(m_displayInterface, &OrgFreedesktopColorHelperDisplayInterface::UpdateSample,
            this, &ColordHelper::UpdateSample);

    // listen to colord sensor events
    auto interface = new CdInterface(QStringLiteral("org.freedesktop.ColorManager"),
                                     QStringLiteral("/org/freedesktop/ColorManager"),
                                     QDBusConnection::systemBus(),
                                     this);
    connect(interface, &CdInterface::SensorAdded, this, &ColordHelper::sensorAdded);
    connect(interface, &CdInterface::SensorRemoved, this, &ColordHelper::sensorRemoved);

    auto async = interface->GetSensors();
    auto watcher = new QDBusPendingCallWatcher(async, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &ColordHelper::gotSensors);
}

ColordHelper::~ColordHelper()
{
    qCDebug(CALIBRATE) << "quiting" << true;

    if (m_started) {
        QDBusPendingReply<void> reply = m_displayInterface->Cancel();
        QThread::sleep(1);
        reply.waitForFinished();
        qCDebug(CALIBRATE) << "canceling calibration" << reply.error();
    }
}

QString ColordHelper::daemonVersion()
{
    return OrgFreedesktopColorHelperInterface(service,
                                              servicePath,
                                              QDBusConnection::sessionBus()).daemonVersion();
}

bool ColordHelper::sensorDetected() const
{
    return m_sensors.size() == 1;
}

uint ColordHelper::progress() const
{
    return m_progress;
}

void ColordHelper::start()
{
    qCDebug(CALIBRATE) << m_quality << m_displayType << m_profileTitle << m_started << m_sensors.size();
    if (m_started || m_sensors.size() != 1) {
        return;
    }
    m_started = true;

    const QString profileId = m_sensors.first().path()
            .section(QLatin1Char('/'), -1, -1).replace(QLatin1Char('_'), QLatin1Char('-'));
    qCDebug(CALIBRATE) << "start using sensor" << profileId;

    auto reply = m_displayInterface->Start(m_deviceId,
                                           profileId,
                                           QVariantMap{
                                               {QStringLiteral("Quality"), m_quality},
                                               {QStringLiteral("Whitepoint"), 0},
                                               {QStringLiteral("Title"), m_profileTitle},
                                               {QStringLiteral("DeviceKind"), m_displayType},
                                               {QStringLiteral("Brightness"), 100}
                                           });
    auto watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &ColordHelper::startCallFinished);
}

void ColordHelper::resume()
{
    m_displayInterface->Resume();
}

void ColordHelper::cancel()
{
    QDBusPendingReply<void> reply = m_displayInterface->Cancel();
    reply.waitForFinished();
    qCDebug(CALIBRATE) << "canceled calibration" << reply.error();
}

void ColordHelper::gotSensors(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<ObjectPathList> reply = *call;
    if (reply.isError()) {
        qCWarning(CALIBRATE) << "Unexpected message" << reply.error().message();
    } else {
        const ObjectPathList sensors = reply.argumentAt<0>();
        for (const QDBusObjectPath &sensor : sensors) {
            // Add the sensors but don't update the Calibrate button
            sensorAdded(sensor);
        }

        // Update the calibrate button later
    }
}

void ColordHelper::sensorAdded(const QDBusObjectPath &object_path)
{
    qCDebug(CALIBRATE) << "sensor added" << object_path.path();
    m_sensors.push_back(object_path);
    Q_EMIT sensorDetectedChanged();
}

void ColordHelper::sensorRemoved(const QDBusObjectPath &object_path)
{
    qCDebug(CALIBRATE) << "sensor removed" << object_path.path();
    m_sensors.removeOne(object_path);
    Q_EMIT sensorDetectedChanged();
}

void ColordHelper::startCallFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (reply.isError()) {
        qCDebug(CALIBRATE) << reply.error();
        m_started = false;
    } else {
        //        qCDebug(CALIBRATE) << reply.error();
    }
    call->deleteLater();
}

void ColordHelper::Finished(uint error_code, const QVariantMap &details)
{
    qCDebug(CALIBRATE) << "finished, error code:" << error_code << details;
}

void ColordHelper::InteractionRequired(uint code, const QString &message, const QString &image)
{
    qCDebug(CALIBRATE) << code << message << image;
    // 0 = attach to screen
    // 1 = move to calibration mode.
    // 2 = move to surface mode.
    // 3 = shut the laptop lid.
    Q_EMIT interaction(code, image);
}

void ColordHelper::UpdateGamma(CdGamaList gamma)
{
    qCDebug(CALIBRATE) << gamma.size();
}

void ColordHelper::UpdateSample(double red, double green, double blue)
{
    qCDebug(CALIBRATE) << red << green << blue;
}
