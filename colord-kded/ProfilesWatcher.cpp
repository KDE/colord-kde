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

#include "ProfilesWatcher.h"

#include "ProfileUtils.h"
#include "Edid.h"

#include <KDirWatch>
#include <KMimeType>
#include <KUser>

#include <QStringBuilder>
#include <QDirIterator>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusUnixFileDescriptor>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusObjectPath>

#include <KDebug>

ProfilesWatcher::ProfilesWatcher(QObject *parent) :
    QThread(parent),
    m_dirWatch(0)
{
}

QString ProfilesWatcher::profilesPath() const
{
    KUser user;
    // ~/.local/share/icc/
    return user.homeDir() % QLatin1String("/.local/share/icc/");
}

void ProfilesWatcher::scanHomeDirectory()
{
    // Get a list of files in ~/.local/share/icc/
    QDir profilesDir(profilesPath());
    if (!profilesDir.exists()) {
        kWarning() << "Icc path" << profilesDir.path() << "does not exist";
        if (!profilesDir.mkpath(profilesPath())) {
            kWarning() << "Failed to create icc path '~/.local/share/icc'";
        }
    }

    //check if any changes to the file occour
    //this also prevents from reading when a checkUpdate happens
    if (!m_dirWatch) {
        m_dirWatch = new KDirWatch(this);
        m_dirWatch->addDir(profilesDir.path(), KDirWatch::WatchFiles);
        connect(m_dirWatch, SIGNAL(created(QString)), this, SLOT(addProfile(QString)));
        connect(m_dirWatch, SIGNAL(deleted(QString)), this, SLOT(removeProfile(QString)));
        m_dirWatch->startScan();
    }

    // Call AddProfile() for each file
    QDirIterator it(profilesDir, QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        addProfile(it.next());
    }
}

void ProfilesWatcher::createIccProfile(bool isLaptop, const Edid &edid)
{
    // EDID profile Creation
    // Creates a path for EDID generated profile
    QString autogenPath = profilesPath();
    QDir profilesDir(autogenPath);
    if (!profilesDir.exists()) {
        kWarning() << "Icc path" << profilesDir.path() << "does not exist";
        if (!profilesDir.mkpath(autogenPath)) {
            kWarning() << "Failed to create icc path '~/.local/share/icc'";
        }
    }
    autogenPath.append(QLatin1String("edid-") % edid.hash() % QLatin1String(".icc"));
    ProfileUtils::createIccProfile(isLaptop, edid, autogenPath);
}

void ProfilesWatcher::addProfile(const QString &filePath)
{
    // if the changed file is an ICC profile
    KMimeType::Ptr mimeType;
    mimeType = KMimeType::findByFileContent(filePath);

    if (!mimeType->is(QLatin1String("application/vnd.iccprofile"))) {
        // not a prifile file
        return;
    }

    // open filename
    QFile profile(filePath);
    if (!profile.open(QIODevice::ReadOnly)) {
        kWarning() << "Failed to open profile file:" << filePath;
        return;
    }

    // get the MD5 hash of the contents
    QString hash;
    hash = ProfileUtils::profileHash(profile);
    // seek(0) so that if we pass the FD to colord it is not at end
    profile.seek(0);

    QString profileId = QLatin1String("icc-") + hash;

    bool fdPass;
    fdPass = (QDBusConnection::systemBus().connectionCapabilities() & QDBusConnection::UnixFileDescriptorPassing);

    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             fdPass ? QLatin1String("CreateProfileWithFd") :
                                                      QLatin1String("CreateProfile"));
    StringStringMap properties;
    properties["Filename"] = filePath;
    properties["FILE_checksum"] = hash;

    message << qVariantFromValue(profileId);
    message << qVariantFromValue(QString("temp"));
    if (fdPass) {
        message << qVariantFromValue(QDBusUnixFileDescriptor(profile.handle()));
    }
    message << qVariantFromValue(properties);

    QDBusReply<QDBusObjectPath> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);

    kDebug() << "created profile" << profileId << reply.value().path();
}

void ProfilesWatcher::removeProfile(const QString &filename)
{
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("FindProfileByFilename"));
    message << qVariantFromValue(filename);
    QDBusReply<QDBusObjectPath> reply = QDBusConnection::systemBus().call(message);

    if (!reply.isValid()) {
        kWarning() << "Could not find the DBus object path for the given file name" << filename;
        return;
    }

    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("DeleteProfile"));
    message << qVariantFromValue(reply.value());
    QDBusConnection::systemBus().send(message);
}
