/***************************************************************************
 *   Copyright (C) 2012 by Daniel Nicoletti                                *
 *   dantti12@gmail.com                                                    *
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
 
#include "ColordEngine.h"

#include "ColordService.h"
  
#include <Plasma/DataContainer>

#include <QStringBuilder>
#include <QFileInfo>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusReply>

#include <KDateTime>

typedef QList<QDBusObjectPath> ObjectPathList;
typedef QMap<QString, QString>  StringStringMap;

#define CD_PROFILE_METADATA_DATA_SOURCE_EDID	 "edid"
#define CD_PROFILE_METADATA_DATA_SOURCE_CALIB	 "calib"
#define CD_PROFILE_METADATA_DATA_SOURCE_STANDARD "standard"
#define CD_PROFILE_METADATA_DATA_SOURCE_TEST     "test"

ColordEngine::ColordEngine(QObject *parent, const QVariantList &args) :
    Plasma::DataEngine(parent, args)
{
    // We ignore any arguments - data engines do not have much use for them
    Q_UNUSED(args)

    qDBusRegisterMetaType<ObjectPathList>();
}

ColordEngine::~ColordEngine()
{
}

void ColordEngine::init()
{
    // Creates a ColorD interface, it must be created with new
    // otherwise the object will be deleted when this block ends
    QDBusInterface *interface;
    interface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                   QLatin1String("/org/freedesktop/ColorManager"),
                                   QLatin1String("org.freedesktop.ColorManager"),
                                   QDBusConnection::systemBus(),
                                   this);
    // listen to colord for device events
    connect(interface, SIGNAL(DeviceAdded(QDBusObjectPath)),
            this, SLOT(deviceAdded(QDBusObjectPath)));
    connect(interface, SIGNAL(DeviceRemoved(QDBusObjectPath)),
            this, SLOT(deviceRemoved(QDBusObjectPath)));
    connect(interface, SIGNAL(DeviceChanged(QDBusObjectPath)),
            this, SLOT(deviceChanged(QDBusObjectPath)));

    // listen to colord for profile events
    connect(interface, SIGNAL(ProfileAdded(QDBusObjectPath)),
            this, SLOT(profileAdded(QDBusObjectPath)));
    connect(interface, SIGNAL(ProfileRemoved(QDBusObjectPath)),
            this, SLOT(profileRemoved(QDBusObjectPath)));
    connect(interface, SIGNAL(ProfileChanged(QDBusObjectPath)),
            this, SLOT(profileChanged(QDBusObjectPath)));

    // Ask for profiles
    QDBusMessage msg;
    msg = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                         QLatin1String("/org/freedesktop/ColorManager"),
                                         QLatin1String("org.freedesktop.ColorManager"),
                                         QLatin1String("GetProfiles"));
    QDBusConnection::systemBus().callWithCallback(msg, this, SLOT(gotProfiles(QDBusMessage)));

    // Ask for devices
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("GetDevices"));
    QDBusConnection::systemBus().callWithCallback(message, this, SLOT(gotDevices(QDBusMessage)));
}

Plasma::Service* ColordEngine::serviceForSource(const QString &source)
{
    return new ColordService(this, source);
}

void ColordEngine::gotDevices(const QDBusMessage &message)
{
    if (message.type() == QDBusMessage::ReplyMessage && message.arguments().size() == 1) {
        QDBusArgument argument = message.arguments().first().value<QDBusArgument>();
        ObjectPathList paths = qdbus_cast<ObjectPathList>(argument);
        foreach (const QDBusObjectPath &path, paths) {
            deviceAdded(path);
        }
    } else {
        kWarning() << "Unexpected message" << message;
    }
}

void ColordEngine::deviceAdded(const QDBusObjectPath &objectPath)
{
    Data sourceData = query(objectPath.path());

    QDBusInterface deviceInterface(QLatin1String("org.freedesktop.ColorManager"),
                                   objectPath.path(),
                                   QLatin1String("org.freedesktop.ColorManager.Device"),
                                   QDBusConnection::systemBus(),
                                   this);
    if (!deviceInterface.isValid()) {
        return;
    }

    QString deviceId = deviceInterface.property("DeviceId").toString();
    QString kind = deviceInterface.property("Kind").toString();
    QString model = deviceInterface.property("Model").toString();
    QString vendor = deviceInterface.property("Vendor").toString();
    QString colorspace = deviceInterface.property("Colorspace").toString();
    ObjectPathList profiles = deviceInterface.property("Profiles").value<ObjectPathList>();

    sourceData[QLatin1String("isDevice")] = true;

    if (kind == QLatin1String("display")) {
        sourceData[QLatin1String("icon")] = QLatin1String("video-display");
    } else if (kind == QLatin1String("scanner")) {
        sourceData[QLatin1String("icon")] = QLatin1String("scanner");
    } else if (kind == QLatin1String("printer")) {
        if (colorspace == QLatin1String("gray")) {
            sourceData[QLatin1String("icon")] = QLatin1String("printer-laser");
        } else {
            sourceData[QLatin1String("icon")] = QLatin1String("printer");
        }
    } else if (kind == QLatin1String("webcam")) {
        sourceData[QLatin1String("icon")] = QLatin1String("camera-web");
    }

    if (model.isEmpty() && vendor.isEmpty()) {
        sourceData[QLatin1String("name")] = deviceId;
    } else if (model.isEmpty()) {
        sourceData[QLatin1String("name")] = vendor;
    } else if (vendor.isEmpty()) {
        sourceData[QLatin1String("name")] = model;
    } else {
        sourceData[QLatin1String("name")] = QString(vendor % QLatin1String(" - ") % model);
    }

    sourceData[QLatin1String("sortRole")] = QString(kind % sourceData[QLatin1String("name")].toString());

    // Convert our Device Kind to Profile Kind
    if (kind == QLatin1String("display")) {
        kind = QLatin1String("display-device");
        sourceData[QLatin1String("kind")] = QLatin1String("display-device");
    } else if (kind == QLatin1String("camera") ||
               kind == QLatin1String("scanner") ||
               kind == QLatin1String("webcam")) {
        sourceData[QLatin1String("kind")] = QLatin1String("input-device");
    } else if (kind == QLatin1String("printer")) {
        sourceData[QLatin1String("kind")] = QLatin1String("output-device");
    } else {
        sourceData[QLatin1String("kind")] = QLatin1String("unknown");
    }

    // Setup the profile list
    QStringList profilesPath;
    foreach (const QDBusObjectPath &profileObjectPath, profiles) {
        profilesPath << profileObjectPath.path();
    }
    sourceData[QLatin1String("profiles")] = profilesPath;

    if (!profilesPath.isEmpty()) {
        sourceData[QLatin1String("defaultProfile")] = profilesPath.first();
    } else {
        sourceData[QLatin1String("defaultProfile")] = QString();
    }

    setData(objectPath.path(), sourceData);
}

void ColordEngine::deviceRemoved(const QDBusObjectPath &objectPath)
{
    removeSource(objectPath.path());
}

void ColordEngine::deviceChanged(const QDBusObjectPath &objectPath)
{
    Q_UNUSED(objectPath)
//    int row = findItem(objectPath);
//    if (row == -1) {
//        kWarning() << "Device not found" << objectPath.path();
//        return;
//    }

//    QDBusInterface deviceInterface(QLatin1String("org.freedesktop.ColorManager"),
//                                   objectPath.path(),
//                                   QLatin1String("org.freedesktop.ColorManager.Device"),
//                                   QDBusConnection::systemBus(),
//                                   this);
//    if (!deviceInterface.isValid()) {
//        return;
//    }

//    ObjectPathList profiles = deviceInterface.property("Profiles").value<ObjectPathList>();

//    // Normally just the profile list bound this device
//    // is what changes including "Modified" property
//    QStandardItem *stdItem = item(row);
//    for (int i = 0; i < profiles.size(); ++i) {
//        // Look for the desired profile
//        QStandardItem *child = findProfile(stdItem, profiles.at(i));
//        if (child) {
//            // Check if the state has changed
//            Qt::CheckState state = i ? Qt::Unchecked : Qt::Checked;
//            if (child->checkState() != state) {
//                child->setCheckState(state);
//            }
//        } else {
//            // Inserts the profile with the parent object Path
//            QStandardItem *profileItem = createProfileItem(profiles.at(i), objectPath, !i);
//            if (profileItem) {
//                stdItem->insertRow(i, profileItem);
//            }
//        }
//    }

//    // Remove the extra items it might have
//    removeProfilesNotInList(stdItem, profiles);

//    emit changed();
}

void ColordEngine::gotProfiles(const QDBusMessage &message)
{
    kDebug() << (message.type() == QDBusMessage::ReplyMessage) << message.arguments().size();
    if (message.type() == QDBusMessage::ReplyMessage && message.arguments().size() == 1) {
        QDBusArgument argument = message.arguments().first().value<QDBusArgument>();
        ObjectPathList paths = qdbus_cast<ObjectPathList>(argument);
        foreach (const QDBusObjectPath &path, paths) {
            profileAdded(path);
        }
    } else {
        kWarning() << "Unexpected message" << message;
    }
}

void ColordEngine::profileAdded(const QDBusObjectPath &objectPath)
{
    Data sourceData = query(objectPath.path());

    QDBusInterface profileInterface(QLatin1String("org.freedesktop.ColorManager"),
                                    objectPath.path(),
                                    QLatin1String("org.freedesktop.ColorManager.Profile"),
                                    QDBusConnection::systemBus(),
                                    this);
    if (!profileInterface.isValid()) {
        return;
    }

    // Verify if the profile has a filename
    QString filename = profileInterface.property("Filename").toString();
    if (filename.isEmpty()) {
        return;
    }

    // Check if we can read the profile
    QFileInfo fileInfo(filename);
    if (!fileInfo.isReadable()) {
        return;
    }

    QString dataSource = getProfileDataSource(objectPath);
    QString profileId = profileInterface.property("ProfileId").toString();
    QString title = profileInterface.property("Title").toString();
    QString kind = profileInterface.property("Kind").toString();
    QString colorspace = profileInterface.property("Colorspace").toString();
    qulonglong created = profileInterface.property("Created").toULongLong();

    // Choose a nice icon
    if (kind == QLatin1String("display-device")) {
        sourceData[QLatin1String("icon")] = QLatin1String("video-display");
    } else if (kind == QLatin1String("input-device")) {
        sourceData[QLatin1String("icon")] = QLatin1String("scanner");
    } else if (kind == QLatin1String("output-device")) {
        if (colorspace == QLatin1String("gray")) {
            sourceData[QLatin1String("icon")] = QLatin1String("printer-laser");
        } else {
            sourceData[QLatin1String("icon")] = QLatin1String("printer");
        }
    } else if (kind == QLatin1String("colorspace-conversion")) {
        sourceData[QLatin1String("icon")] = QLatin1String("view-refresh");
    } else if (kind == QLatin1String("abstract")) {
        sourceData[QLatin1String("icon")] = QLatin1String("insert-link");
    } else if (kind == QLatin1String("named-color")) {
        sourceData[QLatin1String("icon")] = QLatin1String("view-preview");
    } else {
        sourceData[QLatin1String("icon")] = QLatin1String("image-missing");
    }

    // Sets the profile title
    if (title.isEmpty()) {
        title = profileId;
    } else {
        KDateTime createdDT;
        createdDT.setTime_t(created);
        title = profileWithSource(dataSource, title, createdDT);
    }
    sourceData[QLatin1String("display")] = title;

    setData(objectPath.path(), sourceData);
}

void ColordEngine::profileRemoved(const QDBusObjectPath &objectPath)
{
    removeSource(objectPath.path());
}

void ColordEngine::profileChanged(const QDBusObjectPath &objectPath)
{
    Q_UNUSED(objectPath)

    // TODO what should we do when the profile changes?
}

QString ColordEngine::getProfileDataSource(const QDBusObjectPath &objectPath)
{
    QString dataSource;
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             objectPath.path(),
                                             QLatin1String("org.freedesktop.DBus.Properties"),
                                             QLatin1String("Get"));
    message << QString("org.freedesktop.ColorManager.Profile"); // Interface
    message << QString("Metadata"); // Propertie Name
    QDBusReply<QVariant> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);
    if (reply.isValid()) {
        QDBusArgument argument = reply.value().value<QDBusArgument>();
        StringStringMap metadata = qdbus_cast<StringStringMap>(argument);
        if (metadata.contains(QLatin1String("DATA_source"))) {
            dataSource = metadata[QLatin1String("DATA_source")];
        }
    }
    return dataSource;
}

QString ColordEngine::profileWithSource(const QString &dataSource, const QString &profilename, const KDateTime &created)
{
    if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_EDID)) {
        return i18n("Default: %1", profilename);
    } else if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_STANDARD)) {
        return i18n("Colorspace: %1", profilename);
    } else if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_TEST)) {
        return i18n("Test profile: %1", profilename);
    } else if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_CALIB)) {
        return i18n("%1 (%2)",
                    profilename,
                    KGlobal::locale()->formatDateTime(created, KLocale::LongDate));
    }
    return profilename;
}
 
// This does the magic that allows Plasma to load
// this plugin.  The first argument must match
// the X-Plasma-EngineName in the .desktop file.
// The second argument is the name of the class in
// your plugin that derives from Plasma::DataEngine
K_EXPORT_PLASMA_DATAENGINE(plasma_engine_colord, ColordEngine)
 
// this is needed since ColordEngine is a QObject
#include "ColordEngine.moc"
