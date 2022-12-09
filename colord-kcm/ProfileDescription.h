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

#ifndef PROFILEDESCRIPTION_H
#define PROFILEDESCRIPTION_H
#include <QDBusObjectPath>
#include <QObject>

class ProfileMetaDataModel;
class ProfileNamedColorsModel;
class ProfileDescription : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ProfileMetaDataModel *metaDataModel READ metaDataModel CONSTANT)
    Q_PROPERTY(ProfileNamedColorsModel *namedColorsModel READ namedColorsModel CONSTANT)
    Q_PROPERTY(QDBusObjectPath path MEMBER m_currentProfilePath NOTIFY dataChanged)
    Q_PROPERTY(QString kind MEMBER m_profileKind NOTIFY dataChanged)
    Q_PROPERTY(QString colorSpace MEMBER m_profileColorSpace NOTIFY dataChanged)
    Q_PROPERTY(QString version MEMBER m_profileVersion NOTIFY dataChanged)
    Q_PROPERTY(QString createdTime MEMBER m_profileCreatedTime NOTIFY dataChanged)
    Q_PROPERTY(QString license MEMBER m_profileLicense NOTIFY dataChanged)
    Q_PROPERTY(QString manufacturer MEMBER m_profileManufacturer NOTIFY dataChanged)
    Q_PROPERTY(QString model MEMBER m_profileModel NOTIFY dataChanged)
    Q_PROPERTY(QString size MEMBER m_profileSize NOTIFY dataChanged)
    Q_PROPERTY(QString filename MEMBER m_profileName NOTIFY dataChanged)
    Q_PROPERTY(QString whitePoint MEMBER m_profileWhitePoint NOTIFY dataChanged)
    Q_PROPERTY(bool canRemove MEMBER m_canRemoveProfile NOTIFY dataChanged)
    Q_PROPERTY(bool hasDisplayCorrection MEMBER m_profileHasDisplayCorrection NOTIFY dataChanged)

public:
    explicit ProfileDescription(QObject *parent = nullptr);

    ProfileMetaDataModel *metaDataModel() const;
    ProfileNamedColorsModel *namedColorsModel() const;

    Q_INVOKABLE void setProfile(const QDBusObjectPath &objectPath, bool canRemoveProfile);
    Q_INVOKABLE void installSystemWide();

Q_SIGNALS:
    void dataChanged();

private:
    QDBusObjectPath m_currentProfilePath;
    QString m_profileKind;
    QString m_profileColorSpace;
    QString m_profileVersion;
    QString m_profileCreatedTime;
    QString m_profileLicense;
    QString m_profileManufacturer;
    QString m_profileModel;
    QString m_profileSize;
    QString m_profileName;
    QString m_profileWhitePoint;
    bool m_canRemoveProfile;
    bool m_profileHasDisplayCorrection;

    ProfileMetaDataModel *m_metadataModel;
    ProfileNamedColorsModel *m_namedColorsModel;
};

#endif // PROFILEDESCRIPTION_H
