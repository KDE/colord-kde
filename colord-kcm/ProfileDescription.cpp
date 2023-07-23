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
#include <KFormat>

#include "CdProfileInterface.h"
#include "Profile.h"
#include "ProfileDescription.h"
#include "ProfileMetaDataModel.h"
#include "ProfileNamedColorsModel.h"

ProfileDescription::ProfileDescription(QObject *parent)
    : QObject(parent)
    , m_metadataModel(new ProfileMetaDataModel(this))
    , m_namedColorsModel(new ProfileNamedColorsModel(this))
{
}

void ProfileDescription::setProfile(const QDBusObjectPath &objectPath, bool canRemoveProfile)
{
    m_currentProfilePath = objectPath;
    m_canRemoveProfile = canRemoveProfile;

    CdProfileInterface profileInterface(QStringLiteral("org.freedesktop.ColorManager"), objectPath.path(), QDBusConnection::systemBus());
    if (!profileInterface.isValid()) {
        return;
    }

    const QString filename = profileInterface.filename();
    const bool hasVcgt = profileInterface.hasVcgt();
    const qlonglong created = profileInterface.created();

    Profile profile(filename);
    if (profile.loaded()) {
        // Set the profile type
        m_profileKind = profile.kindString();

        // Set the colorspace
        m_profileColorSpace = profile.colorspace();

        // Set the version
        m_profileVersion = profile.version();

        // Set the created time
        m_profileCreatedTime = QLocale().toString(QDateTime::fromSecsSinceEpoch(created), QLocale::LongFormat);

        // Set the license
        m_profileLicense = profile.copyright();

        // Set the manufacturer
        m_profileManufacturer = profile.manufacturer();

        // Set the Model
        m_profileModel = profile.model();

        // Set the Display Correction
        m_profileHasDisplayCorrection = hasVcgt;

        // Set the file size
        m_profileSize = KFormat().formatByteSize(profile.size());

        // Set the file name
        QFileInfo fileinfo(profile.filename());
        m_profileName = fileinfo.fileName();

        QString temp;
        const uint temperature = profile.temperature();
        if (qAbs(temperature - 5000) < 10) {
            temp = QString::fromUtf8("%1K (D50)").arg(QString::number(temperature));
        } else if (qAbs(temperature - 6500) < 10) {
            temp = QString::fromUtf8("%1K (D65)").arg(QString::number(temperature));
        } else {
            temp = QString::fromUtf8("%1K").arg(QString::number(temperature));
        }
        m_profileWhitePoint = temp;

        qDebug() << profile.description();
        qDebug() << profile.model();
        qDebug() << profile.manufacturer();
        qDebug() << profile.copyright();

        // Get named colors
        m_namedColorsModel->setNamedColors(profile.getNamedColors());
        m_metadataModel->setMetadata(profileInterface.metadata());
    }
    qDebug() << profile.filename();
    Q_EMIT dataChanged();
}

ProfileMetaDataModel *ProfileDescription::metaDataModel() const
{
    return m_metadataModel;
}

ProfileNamedColorsModel *ProfileDescription::namedColorsModel() const
{
    return m_namedColorsModel;
}

void ProfileDescription::installSystemWide()
{
    CdProfileInterface profile(QStringLiteral("org.freedesktop.ColorManager"), m_currentProfilePath.path(), QDBusConnection::systemBus());
    profile.InstallSystemWide();
}

#include "moc_ProfileDescription.cpp"
