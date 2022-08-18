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

#include <QWidget>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>

namespace Ui {
    class Description;
}
class CdInterface;
class ProfileNamedColors;
class ProfileMetaData;
class Description : public QWidget
{
    Q_OBJECT
public:
    explicit Description(QWidget *parent = nullptr);
    ~Description() override;

    int innerHeight() const;
    void setCdInterface(CdInterface *interface);
    void setProfile(const QDBusObjectPath &objectPath, bool canRemoveProfile);
    void setDevice(const QDBusObjectPath &objectPath);

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
    void insertTab(int index, QWidget *widget, const QString &label);
    void removeTab(QWidget *widget);
    bool calibrateEnabled(const QString &kind);

    Ui::Description *const ui;
    QDBusObjectPath m_currentProfile;
    QString m_currentDeviceId;
    QString m_currentDeviceKind;
    ProfileNamedColors *m_namedColors = nullptr;
    ProfileMetaData *m_metadata = nullptr;
    QList<QDBusObjectPath> m_sensors;
};

#endif // DESCRIPTION_H
