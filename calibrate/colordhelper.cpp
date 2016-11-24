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

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(CALIBRATE, "calibrate")

static QString service = QStringLiteral("org.freedesktop.ColorHelper");
static QString servicePath = QStringLiteral("/");

ColordHelper::ColordHelper(const QString &deviceId, QObject *parent) : QObject(parent)
  , m_deviceId(deviceId)
{
    m_displayInterface = new OrgFreedesktopColorHelperDisplayInterface(service,
                                                                       servicePath,
                                                                       QDBusConnection::sessionBus());
    connect(m_displayInterface, &OrgFreedesktopColorHelperDisplayInterface::Finished,
            this, &ColordHelper::Finished);
    connect(m_displayInterface, &OrgFreedesktopColorHelperDisplayInterface::InteractionRequired,
            this, &ColordHelper::InteractionRequired);
    connect(m_displayInterface, &OrgFreedesktopColorHelperDisplayInterface::UpdateGamma,
            this, &ColordHelper::UpdateGamma);
    connect(m_displayInterface, &OrgFreedesktopColorHelperDisplayInterface::UpdateSample,
            this, &ColordHelper::UpdateSample);

}

QString ColordHelper::daemonVersion()
{
    return OrgFreedesktopColorHelperInterface(service,
                                              servicePath,
                                              QDBusConnection::sessionBus()).daemonVersion();
}

void ColordHelper::start()
{
    qCDebug(CALIBRATE) << m_quality << m_displayType << m_profileTitle;

    m_profileId = "colorhug-08"; // TODO make this dynamic
    auto reply = m_displayInterface->Start(m_deviceId,
                                           m_profileId,
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

void ColordHelper::startCallFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    if (reply.isError()) {
        qCDebug(CALIBRATE) << reply.error();
    } else {
//        qCDebug(CALIBRATE) << reply.error();
    }
    call->deleteLater();
}

void ColordHelper::Finished(uint error_code, const QVariantMap &details)
{
    qCDebug(CALIBRATE) << error_code << details;
}

void ColordHelper::InteractionRequired(uint code, const QString &message, const QString &image)
{
    qCDebug(CALIBRATE) << code << message << image;
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
