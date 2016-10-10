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

#include "ProfilesWatcher.h"
#include "ProfileUtils.h"
#include "Edid.h"
#include "CdInterface.h"

#include <KDirWatch>

#include <QMimeDatabase>
#include <QMimeType>
#include <QStringBuilder>
#include <QDirIterator>
#include <QDBusUnixFileDescriptor>
#include <QDBusReply>
#include <QDBusObjectPath>
#include <QDebug>

ProfilesWatcher::ProfilesWatcher(QObject *parent) :
    QThread(parent)
{
}

QString ProfilesWatcher::profilesPath() const
{
    // ~/.local/share/icc/
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) % QLatin1String("/icc/");
}

void ProfilesWatcher::scanHomeDirectory()
{
    // Get a list of files in ~/.local/share/icc/
    QDir profilesDir(profilesPath());
    if (!profilesDir.exists()) {
        qWarning() << "Icc path" << profilesDir.path() << "does not exist";
        if (!profilesDir.mkpath(profilesPath())) {
            qWarning() << "Failed to create icc path '~/.local/share/icc'";
        }
    }

    //check if any changes to the file occour
    //this also prevents from reading when a checkUpdate happens
    if (!m_dirWatch) {
        m_dirWatch = new KDirWatch(this);
        m_dirWatch->addDir(profilesDir.path(), KDirWatch::WatchFiles);
        connect(m_dirWatch, &KDirWatch::created, this, &ProfilesWatcher::addProfile);
        connect(m_dirWatch, &KDirWatch::deleted, this, &ProfilesWatcher::removeProfile);
        m_dirWatch->startScan();
    }

    // Call AddProfile() for each file
    QDirIterator it(profilesDir, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        addProfile(it.next());
    }

    emit scanFinished();
}

void ProfilesWatcher::createIccProfile(bool isLaptop, const Edid &edid)
{
    // EDID profile Creation
    // Creates a path for EDID generated profile
    QString autogenPath = profilesPath();
    QDir profilesDir(autogenPath);
    if (!profilesDir.exists()) {
        qWarning() << "Icc path" << profilesDir.path() << "does not exist";
        if (!profilesDir.mkpath(autogenPath)) {
            qWarning() << "Failed to create icc path '~/.local/share/icc'";
        }
    }
    autogenPath.append(QLatin1String("edid-") % edid.hash() % QLatin1String(".icc"));
    ProfileUtils::createIccProfile(isLaptop, edid, autogenPath);
}

void ProfilesWatcher::addProfile(const QString &filePath)
{
    // if the changed file is an ICC profile
    QMimeDatabase db;
    const QMimeType mimeType = db.mimeTypeForFile(filePath);

    if (!mimeType.inherits(QLatin1String("application/vnd.iccprofile"))) {
        // not a profile file
        qWarning() << filePath << "is not an ICC profile";
        return;
    }

    // open filename
    QFile profile(filePath);
    if (!profile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open profile file:" << filePath;
        return;
    }

    // get the MD5 hash of the contents
    QString hash;
    hash = ProfileUtils::profileHash(profile);
    // seek(0) so that if we pass the FD to colord it is not at end
    profile.seek(0);

    QString profileId = QLatin1String("icc-") % hash;
    const CdStringMap properties = {
        {QStringLiteral("Filename"), filePath},
        {QStringLiteral("FILE_checksum"), hash}
    };

    CdInterface cdInterface(QStringLiteral("org.freedesktop.ColorManager"),
                            QStringLiteral("/org/freedesktop/ColorManager"),
                            QDBusConnection::systemBus());

    QDBusReply<QDBusObjectPath> reply;
    // TODO async
    if (QDBusConnection::systemBus().connectionCapabilities() & QDBusConnection::UnixFileDescriptorPassing) {
        reply = cdInterface.CreateProfileWithFd(profileId,
                                                QStringLiteral("temp"),
                                                QDBusUnixFileDescriptor(profile.handle()),
                                                properties);
    } else {
        reply = cdInterface.CreateProfile(profileId,
                                          QStringLiteral("temp"),
                                          properties);
    }

    qDebug() << "created profile" << profileId << reply.value().path();
}

void ProfilesWatcher::removeProfile(const QString &filename)
{
    CdInterface cdInterface(QStringLiteral("org.freedesktop.ColorManager"),
                            QStringLiteral("/org/freedesktop/ColorManager"),
                            QDBusConnection::systemBus());

    // TODO async
    QDBusReply<QDBusObjectPath> reply = cdInterface.FindProfileByFilename(filename);
    if (!reply.isValid()) {
        qWarning() << "Could not find the DBus object path for the given file name" << filename;
        return;
    }

    cdInterface.DeleteProfile(reply);
}
