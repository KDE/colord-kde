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

#include "ColorD.h"

#include "Edid.h"
#include "DmiUtils.h"
#include "ProfileUtils.h"
#include "XEventHandler.h"

#include <KLocale>
#include <KGenericFactory>
#include <KNotification>
#include <KIcon>
#include <KUser>
#include <KDirWatch>
#include <KMimeType>
#include <KApplication>

#include <QX11Info>

#include <QFile>
#include <QDir>
#include <QStringBuilder>
#include <QDBusMetaType>
#include <QDBusUnixFileDescriptor>
#include <QDesktopWidget>
#include <QApplication>
#include <QDesktopWidget>

K_PLUGIN_FACTORY(ColorDFactory, registerPlugin<ColorD>();)
K_EXPORT_PLUGIN(ColorDFactory("colord"))

bool has_1_2 = false;
bool has_1_3 = false;

ColorD::ColorD(QObject *parent, const QVariantList &args) :
    KDEDModule(parent)
{
    // There's not much use for args in a KCM
    Q_UNUSED(args)

    // Register this first or the first time will fail
    qDBusRegisterMetaType<StringStringMap>();
    qDBusRegisterMetaType<QDBusUnixFileDescriptor>();

    /* connect to colord using DBus */
    connectToColorD();

    /* Connect to the display */
    connectToDisplay();

    /* Scan all the *.icc files */
    scanHomeDirectory();

    // This timer makes sure subsequent changes don't
    // keep checking our outputs
//    m_checkOutputsTimer = new QTimer(this);
//    m_checkOutputsTimer->setInterval(600);
//    connect(m_checkOutputsTimer, SIGNAL(timeout()),
//            this, SLOT(checkOutputs()));

//    // The desktop widget infor us about outputs changes
//    QDesktopWidget *desktopWidget = QApplication::desktop();
//    connect(desktopWidget, SIGNAL(screenCountChanged(int)),
//            m_checkOutputsTimer, SLOT(start()));
//    connect(desktopWidget, SIGNAL(resized(int)),
//            m_checkOutputsTimer, SLOT(start()));
//    connect(desktopWidget, SIGNAL(workAreaResized(int)),
//            m_checkOutputsTimer, SLOT(start()));
}

ColorD::~ColorD()
{
}

void ColorD::addProfile(const QFileInfo &fileInfo)
{
    // open filename
    QFile profile(fileInfo.absoluteFilePath());
    if (!profile.open(QIODevice::ReadOnly)) {
        kWarning() << "Failed to open profile file:" << fileInfo.absoluteFilePath();
        return;
    }

    // get the MD5 hash of the contents
    QString hash;
    hash = ProfileUtils::profileHash(profile);
    // seek(0) so that if we pass the FD to colord it is not at end
    profile.seek(0);

    KUser user;
    QString profileId = QLatin1String("icc-") + hash + QLatin1Char('-') + user.loginName();
    kDebug() << "profileId" << profileId;

    bool fdPass;
    fdPass = (QDBusConnection::systemBus().connectionCapabilities() & QDBusConnection::UnixFileDescriptorPassing);

    //TODO: how to save these private to the class?
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             fdPass ? QLatin1String("CreateProfileWithFd") :
                                                      QLatin1String("CreateProfile"));
    StringStringMap properties;
    properties["Filename"] = fileInfo.absoluteFilePath();
    properties["FILE_checksum"] = hash;

    message << qVariantFromValue(profileId);
    message << qVariantFromValue(QString("temp"));
    if (fdPass) {
        message << qVariantFromValue(QDBusUnixFileDescriptor(profile.handle()));
    }
    message << qVariantFromValue(properties);

    QDBusReply<QDBusObjectPath> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);

    kDebug() << "created profile" << reply.value().path();
}

QString ColorD::profilesPath() const
{
    KUser user;
    // ~/.local/share/icc/
    return user.homeDir() % QLatin1String("/.local/share/icc/");
}

void ColorD::scanHomeDirectory()
{
    // Get a list of files in ~/.local/share/icc/
    QDir profilesDir(profilesPath());
    if (!profilesDir.exists()) {
        kWarning() << "Icc path" << profilesDir.path() << "does not exist";
        if (!profilesDir.mkpath(profilesPath())) {
            kWarning() << "Failed to create icc path '~/.local/share/icc'";
        }
    }

    // Call AddProfile() for each file
    foreach (const QFileInfo &fileInfo, profilesDir.entryInfoList(QDir::Files)) {
        KMimeType::Ptr mimeType;
        mimeType = KMimeType::findByFileContent(fileInfo.absoluteFilePath());
        if (mimeType->is(QLatin1String("application/vnd.iccprofile"))) {
            addProfile(fileInfo);
        }
    }

    //check if any changes to the file occour
    //this also prevents from reading when a checkUpdate happens
    KDirWatch *confWatch = new KDirWatch(this);
    confWatch->addDir(profilesDir.path(), KDirWatch::WatchFiles);
    connect(confWatch, SIGNAL(created(QString)), this, SLOT(addProfile(QString)));
    connect(confWatch, SIGNAL(deleted(QString)), this, SLOT(removeProfile(QString)));
    confWatch->startScan();
}

void ColorD::addOutput(Output &output)
{
    QString edidVendor = QLatin1String("unknown");
    QString edidModel = QLatin1String("unknown");
    QString edidSerial = QLatin1String("unknown");
    QString deviceId = QLatin1String("xrandr-unknown");

    // ensure the RROutput is connected
    if (!output.connected()) {
        return;
    }

    Edid edid = output.readEdidData();
    if (edid.isValid()) {
        kDebug() << "Edid Valid" << edid.deviceId(output.name());
        kDebug() << "Edid vendor" << edid.vendor();
        kDebug() << "Edid serial" << edid.serial();
        kDebug() << "Edid name" << edid.name();
        // TODO check if this is a laptop
        if (!edid.vendor().isEmpty()) {
            edidVendor = edid.vendor();
        }
        if (!edid.name().isEmpty()) {
            edidModel = edid.name();
        }
    }

    bool isLaptop = output.isLaptop();
    if (isLaptop) {
        edidModel = DmiUtils::deviceModel();
        edidVendor = DmiUtils::deviceVendor();
        kDebug() << "Output is laptop" << edidModel << edidVendor;
    }

    if (!edid.serial().isEmpty()) {
        edidSerial = edid.serial();
    }

    // grabing the device even if edid is not valid
    // if handles the fallback name if it's not valid
    deviceId = edid.deviceId(output.name());

    /* get the md5 of the EDID blob */
    //TODO

    //TODO: how to save these private to the class?
    StringStringMap properties;
    properties["Kind"] = "display";
    properties["Mode"] = "physical";
    properties["Colorspace"] = "rgb";
    properties["Vendor"] = edidVendor;
    properties["Model"] = edidModel;
    properties["Serial"] = edidSerial;
    properties["XRANDR_name"] = output.name();

    /* call CreateDevice() with a device_id  */
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("CreateDevice"));
    message << qVariantFromValue(deviceId);
    // TODO wheter to use: normal, temp or disk ?
    message << qVariantFromValue(QString("temp"));
    message << qVariantFromValue(properties);
    QDBusReply<QDBusObjectPath> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);
    if (reply.isValid()) {
        /* parse the edid and save in a hash table [m_hash_edid_md5?]*/
        //TODO, and maybe c++ize http://git.gnome.org/browse/gnome-settings-daemon/tree/plugins/color/gcm-edid.c
        output.setPath(reply.value());
        m_connectedOutputs << output;

        QString autogenPath = profilesPath();
        QDir profilesDir(autogenPath);
        if (!profilesDir.exists()) {
            kWarning() << "Icc path" << profilesDir.path() << "does not exist";
            if (!profilesDir.mkpath(autogenPath)) {
                kWarning() << "Failed to create icc path '~/.local/share/icc'";
            }
        }

        autogenPath.append(QLatin1String("edid-") % edid.hash() % QLatin1String(".icc"));
        bool ret;
        ret = ProfileUtils::createIccProfile(isLaptop, edid, autogenPath);
        if (ret == false) {
            kWarning() << "Failed to create auto profile";
        }
    }
    kDebug() << "created device" << reply.value().path();
}

void ColorD::outputChanged(Output &output)
{
    kDebug() << "Device changed" << output.path().path();

    QDBusInterface *deviceInterface;
    deviceInterface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                         output.path().path(),
                                         QLatin1String("org.freedesktop.ColorManager.Device"),
                                         QDBusConnection::systemBus(),
                                         this);
    // check Device.Kind is "display"
    if (deviceInterface->property("Kind").toString() != QLatin1String("display")) {
        // not a display device, ignoring
        deviceInterface->deleteLater();
        return;
    }

    QList<QDBusObjectPath> profiles = deviceInterface->property("Profiles").value<QList<QDBusObjectPath> >();
    if (profiles.isEmpty()) {
        // There are no profiles ignoring
        deviceInterface->deleteLater();
        return;
    }
    deviceInterface->deleteLater();

    // read the default profile (the first path in the Device.Profiles property)
    QDBusObjectPath profileDefault = profiles.first();
    kDebug() << "profileDefault" << profileDefault.path();
    QDBusInterface *profileInterface;
    profileInterface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                          profileDefault.path(),
                                          QLatin1String("org.freedesktop.ColorManager.Profile"),
                                          QDBusConnection::systemBus(),
                                          this);
    QString filename = profileInterface->property("Filename").toString();
    kDebug() << "Default Profile Filename" << filename;
    profileInterface->deleteLater();

    QFile file(filename);
    QByteArray data;
    if (file.open(QIODevice::ReadOnly)) {
        data = file.readAll();
    } else {
        kWarning() << "Failed to open profile" << filename;
        return;
    }

    // read the VCGT data using lcms2
    const cmsToneCurve **vcgt;
    cmsHPROFILE lcms_profile = NULL;

    // open file
    lcms_profile = cmsOpenProfileFromMem((const uint*) data.data(), data.size());
    if (lcms_profile == NULL) {
        kWarning() << "Could not open profile with lcms" << filename;
        return;
    }

    // TODO we need to check when the output got disabled to avoid the X error
//    output.update();

    // The gama size of this output
    int gamaSize = output.getGammaSize();
    if (gamaSize == 0) {
        kWarning() << "Gamma size is zero";
        return;
    }

    // Allocate the gamma
    XRRCrtcGamma *gamma = XRRAllocGamma(gamaSize);

    // get tone curves from profile
    vcgt = static_cast<const cmsToneCurve **>(cmsReadTag(lcms_profile, cmsSigVcgtTag));
    if (vcgt == NULL || vcgt[0] == NULL) {
        kDebug() << "Profile does not have any VCGT data, reseting";
        // Reset the gamma table
        for (int i = 0; i < gamaSize; ++i) {
            uint value = (i * 0xffff) / (gamaSize - 1);
            gamma->red[i]   = value;
            gamma->green[i] = value;
            gamma->blue[i]  = value;
        }
    } else {
        // Fill the gamma table with the VCGT data
        for (int i = 0; i < gamaSize; ++i) {
            cmsFloat32Number in;
            in = (double) i / (double) (gamaSize - 1);
            gamma->red[i]   = cmsEvalToneCurveFloat(vcgt[0], in) * (double) 0xffff;
            gamma->green[i] = cmsEvalToneCurveFloat(vcgt[1], in) * (double) 0xffff;
            gamma->blue[i]  = cmsEvalToneCurveFloat(vcgt[2], in) * (double) 0xffff;
        }
    }
    cmsCloseProfile(lcms_profile);

    // push the data to the Xrandr gamma ramps for the display
    output.setGamma(gamma);

    XRRFreeGamma(gamma);

    // export the file data as an x atom on the *screen* (not output)
    Atom prop = XInternAtom(m_dpy, "_ICC_PROFILE", true);
    int rc = XChangeProperty(m_dpy,
                             m_root,
                             prop,
                             XA_CARDINAL,
                             8,
                             PropModeReplace,
                             (unsigned char *) data.data(),
                             data.size());

    // for some reason this fails with BadRequest, but actually sets the value
    if (rc != BadRequest && rc != Success) {
        kWarning() << "Failed to set XProperty";
    }
}

void ColorD::removeOutput(const Output &output)
{
    /* call DBus DeleteDevice() on the output */
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("DeleteDevice"));
    message << qVariantFromValue(output.path()); // Profile Path

    /* call ColorManager.DeleteDevice() with the device and profile object paths */
    QDBusConnection::systemBus().send(message);

    // Remove the output from the connected list
    m_connectedOutputs.removeOne(output);
}

void ColorD::connectToDisplay()
{
    m_dpy = QX11Info::display();

    // Check extension
    if (XRRQueryExtension(m_dpy, &m_eventBase, &m_errorBase) == false) {
        m_valid = false;
        return;
    }

    // Install our X event handler
    m_eventHandler = new XEventHandler(m_eventBase);
    connect(m_eventHandler, SIGNAL(outputChanged()),
            this, SLOT(checkOutputs()));
    kapp->installX11EventFilter(m_eventHandler);

    int major_version, minor_version;
    XRRQueryVersion(m_dpy, &major_version, &minor_version);

    m_version = i18n("X Resize and Rotate extension version %1.%2",
                     major_version,minor_version);

    // check if we have the new version of the XRandR extension
    has_1_2 = (major_version > 1 || (major_version == 1 && minor_version >= 2));
    has_1_3 = (major_version > 1 || (major_version == 1 && minor_version >= 3));

    if (has_1_3) {
        kDebug() << "Using XRANDR extension 1.3 or greater.";
    } else if (has_1_2) {
        kDebug() << "Using XRANDR extension 1.2.";
    } else {
        kDebug() << "Using legacy XRANDR extension (1.1 or earlier).";
    }

    kDebug() << "XRANDR error base: " << m_errorBase;

    /* XRRGetScreenResourcesCurrent is less expensive than
     * XRRGetScreenResources, however it is available only
     * in RandR 1.3 or higher
     */
    m_root = RootWindow(m_dpy, 0);

    if (has_1_3) {
        m_resources = XRRGetScreenResourcesCurrent(m_dpy, m_root);
    } else {
        m_resources = XRRGetScreenResources(m_dpy, m_root);
    }

    for (int i = 0; i < m_resources->noutput; ++i) {
        kDebug() << "Adding" << m_resources->outputs[i];
        Output output(m_resources->outputs[i], m_resources);
        addOutput(output);
    }
}

void ColorD::checkOutputs()
{
    kDebug();
//    m_checkOutputsTimer->stop();
    // Check the output as something has changed
    for (int i = 0; i < m_resources->noutput; ++i) {
        Output currentOutput(m_resources->outputs[i], m_resources);
        int i = m_connectedOutputs.indexOf(currentOutput);
        if (i != -1) {
            if (currentOutput.connected()) {
                // Maybe the resolution changed
                // TODO should we store the resolution
                // if so is the resolution enough?
                kDebug() << "device changed";
                deviceChanged(m_connectedOutputs.at(i).path());
            } else {
                // remove the device
                kDebug() << "device removed";
                removeOutput(m_connectedOutputs.at(i));
            }
        } else {
            // Output not found
            addOutput(currentOutput);
        }
    }
}

void ColorD::profileAdded(const QDBusObjectPath &objectPath)
{
    /* check if the EDID_md5 Profile.Metadata matches any connected
     * XRandR devices (e.g. lvds1), otherwise ignore */
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             objectPath.path(),
                                             QLatin1String("org.freedesktop.DBus.Properties"),
                                             QLatin1String("Get"));
    message << QString("org.freedesktop.ColorManager.Profile"); // Interface
    message << QString("Metadata"); // Propertie Name
    QDBusReply<QVariant> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);
    if (!reply.isValid()) {
        kWarning() << "Failed to get Metadata from profile" << objectPath.path() << reply.error().message();
        return;
    }
    QDBusArgument argument = reply.value().value<QDBusArgument>();
    StringStringMap metadata = qdbus_cast<StringStringMap>(argument);
    kDebug() << reply.value() << metadata;
    kDebug() << metadata.size();

    StringStringMap::const_iterator i = metadata.constBegin();
    while (i != metadata.constEnd()) {
        kDebug() << i.key() << ": " << i.value();
        if (i.key() == QLatin1String("EDID_md5")) {
            QString edidHash = i.value();
            const Output *output = 0;
            // Get the Crtc of this output
            foreach (const Output &out, m_connectedOutputs) {
                if (out.edidHash() == edidHash) {
                    output = &out;
                    break;
                }
            }

            if (output) {
                // Found an EDID that matches the md5
                QDBusMessage message;
                message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                                         output->path().path(),
                                                         QLatin1String("org.freedesktop.ColorManager.Device"),
                                                         QLatin1String("AddProfile"));
                message << QString("soft"); // Relation
                message << qVariantFromValue(objectPath); // Profile Path

                /* call Device.AddProfile() with the device and profile object paths */
                QDBusConnection::systemBus().send(message);
                kDebug() << "Profile added" << output->path().path() << objectPath.path();
            }
        }
        ++i;
    }
}

void ColorD::deviceAdded(const QDBusObjectPath &objectPath)
{
    kDebug() << "Device added" << objectPath.path();

    /* show a notification that the user should calibrate the device */
    //TODO
}

void ColorD::deviceChanged(const QDBusObjectPath &objectPath)
{
    kDebug() << "Device changed" << objectPath.path();
    Output *output = 0;
    // Get the Crtc of this output
    foreach (const Output &out, m_connectedOutputs) {
        if (out.path() == objectPath) {
            output = const_cast<Output*>(&out);
            break;
        }
    }

    if (!output) {
        kWarning() << "Output not found";
        return;
    }

    outputChanged(*output);
}

void ColorD::addProfile(const QString &filename)
{
    kDebug() << filename;
    QFileInfo fileInfo(filename);
    addProfile(fileInfo);
}

void ColorD::removeProfile(const QString &filename)
{
    kDebug() << filename;
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("FindProfileByFilename"));
    message << qVariantFromValue(filename);
    QDBusReply<QDBusObjectPath> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);

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

void ColorD::connectToColorD()
{
    // Creates a ColorD interface, it must be created with new
    // otherwise the object will be deleted when this block ends
    QDBusInterface *interface;
    interface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                   QLatin1String("/org/freedesktop/ColorManager"),
                                   QLatin1String("org.freedesktop.ColorManager"),
                                   QDBusConnection::systemBus(),
                                   this);

    // listen to colord for events
    connect(interface, SIGNAL(ProfileAdded(QDBusObjectPath)),
            this, SLOT(profileAdded(QDBusObjectPath)));
    connect(interface, SIGNAL(DeviceAdded(QDBusObjectPath)),
            this, SLOT(deviceAdded(QDBusObjectPath)));
    connect(interface, SIGNAL(DeviceChanged(QDBusObjectPath)),
            this, SLOT(deviceChanged(QDBusObjectPath)));
}
