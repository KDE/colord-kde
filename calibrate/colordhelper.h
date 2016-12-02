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
#ifndef COLORDHELPER_H
#define COLORDHELPER_H

#include <QObject>
#include <QVector>
#include <QDBusObjectPath>

#include <dbus-types.h>

class OrgFreedesktopColorHelperInterface;
class OrgFreedesktopColorHelperDisplayInterface;

class QDBusPendingCallWatcher;

class ColordHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint quality MEMBER m_quality NOTIFY qualityChanged)
    Q_PROPERTY(uint displayType MEMBER m_displayType NOTIFY displayTypeChanged)
    Q_PROPERTY(QString profileTitle MEMBER m_profileTitle NOTIFY profileTitleChanged)
    Q_PROPERTY(bool sensorDetected READ sensorDetected NOTIFY sensorDetectedChanged)
public:
    explicit ColordHelper(const QString &deviceId, QObject *parent = 0);
    ~ColordHelper();

    static QString daemonVersion();

    bool sensorDetected() const;

    Q_PROPERTY(uint Progress READ progress NOTIFY progressChanged)
    uint progress() const;

public Q_SLOTS:
    void start();
    void resume();
    void cancel();

private Q_SLOTS:
    void gotSensors(QDBusPendingCallWatcher *call);
    void sensorAdded(const QDBusObjectPath &object_path);
    void sensorRemoved(const QDBusObjectPath &object_path);

    void startCallFinished(QDBusPendingCallWatcher *call);
    void Finished(uint error_code, const QVariantMap &details);
    void InteractionRequired(uint code, const QString &message, const QString &image);
    void UpdateGamma(CdGamaList gamma);
    void UpdateSample(double red, double green, double blue);

Q_SIGNALS:
    void progressChanged();
    void qualityChanged();
    void displayTypeChanged();
    void profileTitleChanged();
    void sensorDetectedChanged();
    void interaction(uint code, const QString &image);

private:
    OrgFreedesktopColorHelperDisplayInterface *m_displayInterface;
    QVector<QDBusObjectPath> m_sensors;
    QString m_deviceId;
    QString m_profileTitle;
    bool m_started = false;
    uint m_quality = 0;
    uint m_displayType = 0;
    uint m_progress = 0;

};

#endif // COLORDHELPER_H
