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

#include "ProfilesWatcher.h"
#include "XEventHandler.h"
#include "DmiUtils.h"

#include <lcms2.h>

#include <QFile>
#include <QStringBuilder>
#include <QX11Info>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusServiceWatcher>
#include <QtDBus/QDBusUnixFileDescriptor>
#include <QtDBus/QDBusReply>

#include <KGenericFactory>

K_PLUGIN_FACTORY(ColorDFactory, registerPlugin<ColorD>();)
K_EXPORT_PLUGIN(ColorDFactory("colord"))

typedef QList<QDBusObjectPath> ObjectPathList;

ColorD::ColorD(QObject *parent, const QVariantList &args) :
    KDEDModule(parent),
    m_x11EventHandler(0),
    m_profilesWatcher(0)
{
    // There's not much use for args in a KDED
    Q_UNUSED(args)

    // Register this first or the first time will fail
    qDBusRegisterMetaType<StringStringMap>();
    qDBusRegisterMetaType<QDBusUnixFileDescriptor>();
    qDBusRegisterMetaType<ObjectPathList>();
    qRegisterMetaType<Edid>();

    // connect to colord using DBus
    connectToColorD();

    // Connect to the display
    if ((m_resources = connectToDisplay()) == 0) {
        kWarning() << "Failed to connect to DISPLAY and get the needed resources";
        return;
    }

    // Make sure we know is colord is running
    QDBusServiceWatcher *watcher;
    watcher = new QDBusServiceWatcher("org.freedesktop.ColorManager",
                                      QDBusConnection::systemBus(),
                                      QDBusServiceWatcher::WatchForOwnerChange,
                                      this);
    connect(watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)));

    // Create the profiles watcher thread
    m_profilesWatcher = new ProfilesWatcher;
    m_profilesWatcher->start();

    // init the settings
    init();
}

ColorD::~ColorD()
{
    if (m_x11EventHandler) {
        m_x11EventHandler->deleteLater();
    }

    // Stop the thread
    m_profilesWatcher->quit();
    m_profilesWatcher->wait();
    m_profilesWatcher->deleteLater();
}

void ColorD::init()
{
    // Scan all the *.icc files later on it's own thread as this takes quite some time
    QMetaObject::invokeMethod(m_profilesWatcher,
                              "scanHomeDirectory",
                              Qt::QueuedConnection);

    // Check outputs add all connected outputs
    checkOutputs();
}

void ColorD::reset()
{
    m_connectedOutputs.clear();
}

void ColorD::addProfileToDevice(const QDBusObjectPath &profilePath, const QDBusObjectPath &devicePath)
{
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             devicePath.path(),
                                             QLatin1String("org.freedesktop.ColorManager.Device"),
                                             QLatin1String("AddProfile"));
    message << QString("soft"); // Relation
    message << qVariantFromValue(profilePath); // Profile Path

    /* call Device.AddProfile() with the device and profile object paths */
    QDBusConnection::systemBus().send(message);
    kDebug() << "Profile added" << devicePath.path() << profilePath.path();
}

void ColorD::addEdidProfileToDevice(Output &output)
{
    // Ask for profiles
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("GetProfiles"));
    QDBusReply<ObjectPathList> paths = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);

    // Search through all profiles to see if the edid md5 matches
    foreach (const QDBusObjectPath &profilePath, paths.value()) {
        StringStringMap metadata = getProfileMetadata(profilePath);
        if (metadata.contains(QLatin1String("EDID_md5"))) {
            // check if md5 matches
            if (metadata[QLatin1String("EDID_md5")] == output.edidHash()) {
                kDebug() << "Foud EDID profile for device" << profilePath.path() << output.path().path();
                addProfileToDevice(profilePath, output.path());
            }
        }
    }
}

StringStringMap ColorD::getProfileMetadata(const QDBusObjectPath &profilePath)
{
    StringStringMap ret;
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             profilePath.path(),
                                             QLatin1String("org.freedesktop.DBus.Properties"),
                                             QLatin1String("Get"));
    message << QString("org.freedesktop.ColorManager.Profile"); // Interface
    message << QString("Metadata"); // Propertie Name
    QDBusReply<QVariant> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);
    if (!reply.isValid()) {
        kWarning() << "Failed to get Metadata from profile" << profilePath.path() << reply.error().message();
        return ret;
    }

    QDBusArgument argument = reply.value().value<QDBusArgument>();
    ret = qdbus_cast<StringStringMap>(argument);
    return ret;
}

void ColorD::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(serviceName)
    kDebug() << oldOwner << newOwner;
    if (newOwner.isEmpty()) {
        // colord has quit
        reset();
    } else if (oldOwner != newOwner) {
        // colord has a new owner
        reset();
        init();
    } else {
        // colord has started
        init();
    }
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

    // Check if the output is the laptop panel
    bool isLaptop = output.isLaptop();

    Edid edid = output.readEdidData();
    // If it's not the laptop panel and the edid is valid grab the edid info
    if (!isLaptop && edid.isValid()) {
        if (!edid.vendor().isEmpty()) {
            // Store the monitor vendor
            edidVendor = edid.vendor();
        }
        if (!edid.name().isEmpty()) {
            // Store the monitor model
            edidModel = edid.name();
        }
    } else if (isLaptop) {
        // Use the DMI info when on a laptop
        edidModel = DmiUtils::deviceModel();
        edidVendor = DmiUtils::deviceVendor();
    } else {
        // Fallback to the output name
        edidModel = output.name();
    }

    if (!edid.serial().isEmpty()) {
        // Store the EDID serial number
        edidSerial = edid.serial();
    }

    // grabing the device even if edid is not valid
    // if handles the fallback name if it's not valid
    deviceId = output.id();

    // Creates the default profile
    QMetaObject::invokeMethod(m_profilesWatcher,
                              "createIccProfile",
                              Qt::QueuedConnection,
                              Q_ARG(bool, isLaptop),
                              Q_ARG(Edid, edid));

    // build up a map with the output properties to send to colord
    StringStringMap properties;
    properties["Kind"] = QLatin1String("display");
    properties["Mode"] = QLatin1String("physical");
    properties["Colorspace"] = QLatin1String("rgb");
    properties["Vendor"] = edidVendor;
    properties["Model"] = edidModel;
    properties["Serial"] = edidSerial;
    properties["XRANDR_name"] = output.name();

    // call CreateDevice() with a device_id
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("/org/freedesktop/ColorManager"),
                                             QLatin1String("org.freedesktop.ColorManager"),
                                             QLatin1String("CreateDevice"));
    message << qVariantFromValue(deviceId);
    // We use temp because if we crash or quit the device gets removedf
    message << qVariantFromValue(QString("temp"));
    message << qVariantFromValue(properties);
    kDebug() << "Adding device id:" << deviceId;
    kDebug() << "Output Hash:" << output.edidHash();
    QDBusReply<QDBusObjectPath> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);
    if (reply.isValid()) {
        kDebug() << "Created colord device" << reply.value().path();
        // Store the output path into our Output class
        output.setPath(reply.value());

        // Store the connected output into the connected list
        m_connectedOutputs << output;

        // Check if there is any EDID profile to be added
        addEdidProfileToDevice(output);
    } else {
        kWarning() << "Failed to register device:" << reply.error().message();
    }
}

void ColorD::outputChanged(Output &output)
{
    kDebug() << "Device changed" << output.path().path();

    QDBusInterface deviceInterface(QLatin1String("org.freedesktop.ColorManager"),
                                   output.path().path(),
                                   QLatin1String("org.freedesktop.ColorManager.Device"),
                                   QDBusConnection::systemBus(),
                                   this);
    if (!deviceInterface.isValid()) {
        return;
    }

    // check Device.Kind is "display"
    if (deviceInterface.property("Kind").toString() != QLatin1String("display")) {
        // not a display device, ignoring
        return;
    }

    QList<QDBusObjectPath> profiles = deviceInterface.property("Profiles").value<QList<QDBusObjectPath> >();
    if (profiles.isEmpty()) {
        // There are no profiles ignoring
        return;
    }

    // read the default profile (the first path in the Device.Profiles property)
    QDBusObjectPath profileDefault = profiles.first();
    kDebug() << "profileDefault" << profileDefault.path();
    QDBusInterface profileInterface(QLatin1String("org.freedesktop.ColorManager"),
                                    profileDefault.path(),
                                    QLatin1String("org.freedesktop.ColorManager.Profile"),
                                    QDBusConnection::systemBus(),
                                    this);
    if (!profileInterface.isValid()) {
        return;
    }
    QString filename = profileInterface.property("Filename").toString();
    kDebug() << "Default Profile Filename" << filename;

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

    // The gama size of this output
    int gamaSize = output.getGammaSize();
    if (gamaSize == 0) {
        kWarning() << "Gamma size is zero";
        cmsCloseProfile(lcms_profile);
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

    bool isPrimary = false;
    if (output.isPrimary(m_has_1_3, m_root)) {
        isPrimary = true;
    } else {
        // if we find another primary output we are sure
        // this is not the primary one..
        bool foundPrimary = false;
        foreach (const Output &out, m_connectedOutputs) {
            if (out.isPrimary(m_has_1_3, m_root)) {
                foundPrimary = true;
                break;
            }
        }

        // We did not found the primary
        if (!foundPrimary) {
            if (output.isLaptop()) {
                // there are no other primary outputs
                // in a Laptop's case it's probably the one
                isPrimary = true;
            } else if (!m_connectedOutputs.isEmpty()) {
                // Last resort just take the first connected
                // output
                isPrimary = m_connectedOutputs.first() == output;
            }
        }
    }

    if (isPrimary) {
        kDebug() << "Setting X atom on output:"  << output.name();
        // export the file data as an x atom on PRIMARY *screen* (not output)
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

XRRScreenResources *ColorD::connectToDisplay()
{
    m_dpy = QX11Info::display();

    // Check extension
    int eventBase;
    int major_version, minor_version;
    if (!XRRQueryExtension(m_dpy, &eventBase, &m_errorBase) ||
            !XRRQueryVersion(m_dpy, &major_version, &minor_version))
    {
        kWarning() << "RandR extension missing";
        return 0;
    }

    // Install our X event handler
    m_x11EventHandler = new XEventHandler(eventBase);
    connect(m_x11EventHandler, SIGNAL(outputChanged()),
            this, SLOT(checkOutputs()));

    // check if we have the new version of the XRandR extension
    bool has_1_2;
    has_1_2 = (major_version > 1 || (major_version == 1 && minor_version >= 2));
    m_has_1_3 = (major_version > 1 || (major_version == 1 && minor_version >= 3));

    if (m_has_1_3) {
        kDebug() << "Using XRANDR extension 1.3 or greater.";
    } else if (has_1_2) {
        kDebug() << "Using XRANDR extension 1.2.";
    } else {
        kDebug() << "Using legacy XRANDR extension (1.1 or earlier).";
    }

    m_root = RootWindow(m_dpy, 0);

    // This call is CRITICAL:
    // RR 1.3 has a new method called XRRGetScreenResourcesCurrent,
    // that method is less expensive since it only gets the current
    // cached values from X. On the other hand in the case of
    // a session startup the EDID of the output is not cached, leading
    // to our code failing to get a valid EDID. This call ensures the
    // X server will probe all outputs again and cache the EDID.
    // Note: This code only runs once so it's nothing that would
    // actually slow things down.
    return XRRGetScreenResources(m_dpy, m_root);
}

void ColorD::checkOutputs()
{
    kDebug();
    // Check the output as something has changed
    for (int i = 0; i < m_resources->noutput; ++i) {
        Output currentOutput(m_resources->outputs[i], m_resources);
        int index = m_connectedOutputs.indexOf(currentOutput);
        if (index != -1) {
            if (!currentOutput.connected()) {
                // The device is not connected anymore
                kDebug() << "remove device";
                removeOutput(m_connectedOutputs.at(index));
            }
        } else if (currentOutput.connected()) {
            // Output is now connected
            addOutput(currentOutput);
        }
    }
}

void ColorD::profileAdded(const QDBusObjectPath &profilePath)
{
    // check if the EDID_md5 Profile.Metadata matches any connected
    // XRandR devices (e.g. lvds1), otherwise ignore
    StringStringMap metadata = getProfileMetadata(profilePath);
    if (metadata.contains(QLatin1String("EDID_md5"))) {
        QString edidHash = metadata[QLatin1String("EDID_md5")];
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
            addProfileToDevice(profilePath, output->path());
        }
    }
}

void ColorD::deviceAdded(const QDBusObjectPath &objectPath)
{
    kDebug() << "Device added" << objectPath.path();
//    QDBusInterface deviceInterface(QLatin1String("org.freedesktop.ColorManager"),
//                                   objectPath.path(),
//                                   QLatin1String("org.freedesktop.ColorManager.Device"),
//                                   QDBusConnection::systemBus(),
//                                   this);
//    if (!deviceInterface.isValid()) {
//        return;
//    }

//    // check Device.Kind is "display"
//    if (deviceInterface.property("Kind").toString() != QLatin1String("display")) {
//        // not a display device, ignoring
//        return;
//    }

    /* show a notification if the user should calibrate the device */
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
