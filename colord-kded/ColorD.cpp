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

#include "CdInterface.h"
#include "CdDeviceInterface.h"
#include "CdProfileInterface.h"

#include <lcms2.h>

#include <QFile>
#include <QStringBuilder>
#include <QX11Info>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusConnection>
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
    qRegisterMetaType<CdStringMap>();
    qDBusRegisterMetaType<CdStringMap>();
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
    watcher = new QDBusServiceWatcher(QLatin1String("org.freedesktop.ColorManager"),
                                      QDBusConnection::systemBus(),
                                      QDBusServiceWatcher::WatchForOwnerChange,
                                      this);
    connect(watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)));

    // Create the profiles watcher thread
    m_profilesWatcher = new ProfilesWatcher;
    m_profilesWatcher->start();

    // Check outputs add all connected outputs, once profiles are ready
    connect(m_profilesWatcher, SIGNAL(scanFinished()),
            this, SLOT(checkOutputs()), Qt::QueuedConnection);

    // init the settings
    init();
}

ColorD::~ColorD()
{
    foreach (const Output::Ptr &out, m_connectedOutputs) {
        removeOutput(out);
    }

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
}

void ColorD::reset()
{
    m_connectedOutputs.clear();
}

void ColorD::addEdidProfileToDevice(const Output::Ptr &output)
{
    // Ask for profiles
    QDBusReply<ObjectPathList> paths = m_cdInterface->GetProfiles();

    // Search through all profiles to see if the edid md5 matches
    foreach (const QDBusObjectPath &profilePath, paths.value()) {
        CdStringMap metadata = getProfileMetadata(profilePath);
        if (metadata.contains(QLatin1String("EDID_md5"))) {
            // check if md5 matches
            if (metadata[QLatin1String("EDID_md5")] == output->edidHash()) {
                kDebug() << "Found EDID profile for device" << profilePath.path() << output->name();
                if (output->interface()) {
                    output->interface()->AddProfile(QLatin1String("soft"), profilePath);
                }
            }
        }
    }
}

CdStringMap ColorD::getProfileMetadata(const QDBusObjectPath &profilePath)
{
    CdProfileInterface profile(QLatin1String("org.freedesktop.ColorManager"),
                               profilePath.path(),
                               QDBusConnection::systemBus());
    return profile.metadata();
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

void ColorD::addOutput(const Output::Ptr &output)
{
    QString edidVendor = QLatin1String("unknown");
    QString edidModel = QLatin1String("unknown");
    QString edidSerial = QLatin1String("unknown");
    QString deviceId = QLatin1String("xrandr-unknown");

    // ensure the RROutput is connected
    if (!output->connected()) {
        return;
    }

    // Check if the output is the laptop panel
    bool isLaptop = output->isLaptop();

    Edid edid = output->readEdidData();
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
        edidModel = output->name();
    }

    if (!edid.serial().isEmpty()) {
        // Store the EDID serial number
        edidSerial = edid.serial();
    }

    // grabing the device even if edid is not valid
    // if handles the fallback name if it's not valid
    deviceId = output->id();

    // Creates the default profile
    QMetaObject::invokeMethod(m_profilesWatcher,
                              "createIccProfile",
                              Qt::QueuedConnection,
                              Q_ARG(bool, isLaptop),
                              Q_ARG(Edid, edid));

    // build up a map with the output properties to send to colord
    CdStringMap properties;
    properties["Kind"] = QLatin1String("display");
    properties["Mode"] = QLatin1String("physical");
    properties["Colorspace"] = QLatin1String("rgb");
    properties["Vendor"] = edidVendor;
    properties["Model"] = edidModel;
    properties["Serial"] = edidSerial;
    properties["XRANDR_name"] = output->name();

    // We use temp because if we crash or quit the device gets removed
    kDebug() << "Adding device id:" << deviceId;
    kDebug() << "Output Hash:" << output->edidHash();
    QDBusReply<QDBusObjectPath> reply = m_cdInterface->CreateDevice(deviceId,
                                                                    QLatin1String("temp"),
                                                                    properties);
    if (reply.isValid()) {
        kDebug() << "Created colord device" << reply.value().path();
        // Store the output path into our Output class
        output->setPath(reply.value());

        // Store the connected output into the connected list
        m_connectedOutputs << output;

        // Check if there is any EDID profile to be added
        addEdidProfileToDevice(output);

        // Make sure we set the profile on this device
        outputChanged(output);
    } else {
        kWarning() << "Failed to register device:" << reply.error().message();
    }
}

void ColorD::outputChanged(const Output::Ptr &output)
{
    kDebug() << "Device changed" << output->path().path();

    if (!output->interface()) {
        return;
    }

    // check Device.Kind is "display"
    if (output->interface()->kind() != QLatin1String("display")) {
        // not a display device, ignoring
        return;
    }

    QList<QDBusObjectPath> profiles = output->interface()->profiles();
    if (profiles.isEmpty()) {
        // There are no profiles ignoring
        return;
    }

    // read the default profile (the first path in the Device.Profiles property)
    QDBusObjectPath profileDefault = profiles.first();
    kDebug() << "profileDefault" << profileDefault.path();
    CdProfileInterface profile(QLatin1String("org.freedesktop.ColorManager"),
                               profileDefault.path(),
                               QDBusConnection::systemBus());
    if (!profile.isValid()) {
        return;
    }
    QString filename = profile.filename();
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
    int gamaSize = output->getGammaSize();
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
    output->setGamma(gamma);

    XRRFreeGamma(gamma);

    bool isPrimary = false;
    if (output->isPrimary(m_has_1_3, m_root)) {
        isPrimary = true;
    } else {
        // if we find another primary output we are sure
        // this is not the primary one..
        bool foundPrimary = false;
        foreach (const Output::Ptr &out, m_connectedOutputs) {
            if (out->isPrimary(m_has_1_3, m_root)) {
                foundPrimary = true;
                break;
            }
        }

        // We did not found the primary
        if (!foundPrimary) {
            if (output->isLaptop()) {
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
        kDebug() << "Setting X atom on output:"  << output->name();
        // export the file data as an x atom on PRIMARY *screen* (not output)
        Atom prop = XInternAtom(m_dpy, "_ICC_PROFILE", false);
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

void ColorD::removeOutput(const Output::Ptr &output)
{
    /* call DBus DeleteDevice() on the output */
    m_cdInterface->DeleteDevice(output->path());

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
        bool found = false;
        Output::Ptr currentOutput(new Output(m_resources->outputs[i], m_resources));
        foreach (const Output::Ptr &output, m_connectedOutputs) {
            if (output->output() == m_resources->outputs[i]) {
                if (!currentOutput->connected()) {
                    // The device is not connected anymore
                    kDebug() << "remove device";
                    removeOutput(output);
                    found = true;
                    break;
                }
            }
        }

        if (!found && currentOutput->connected()) {
            // Output is now connected
            addOutput(currentOutput);
        }
    }
}

void ColorD::profileAdded(const QDBusObjectPath &profilePath)
{
    // check if the EDID_md5 Profile.Metadata matches any connected
    // XRandR devices (e.g. lvds1), otherwise ignore
    CdStringMap metadata = getProfileMetadata(profilePath);
    if (metadata.contains(QLatin1String("EDID_md5"))) {
        QString edidHash = metadata[QLatin1String("EDID_md5")];
        Output::Ptr output;
        // Get the Crtc of this output
        for (int i = 0; i < m_connectedOutputs.size(); ++i) {
            if (m_connectedOutputs.at(i)->edidHash() == edidHash) {
                output = m_connectedOutputs[i];
                break;
            }
        }

        if (output && output->interface()) {
            // Found an EDID that matches the md5
            output->interface()->AddProfile(QLatin1String("soft"), profilePath);
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
    Output::Ptr output;
    // Get the Crtc of this output
    for (int i = 0; i < m_connectedOutputs.size(); ++i) {
        if (m_connectedOutputs.at(i)->path() == objectPath) {
            output = m_connectedOutputs[i];
            break;
        }
    }

    if (output.isNull()) {
        kWarning() << "Output not found";
        return;
    }

    outputChanged(output);
}

void ColorD::connectToColorD()
{
    // Creates a ColorD interface, it must be created with new
    // otherwise the object will be deleted when this block ends
    m_cdInterface = new CdInterface(QLatin1String("org.freedesktop.ColorManager"),
                                    QLatin1String("/org/freedesktop/ColorManager"),
                                    QDBusConnection::systemBus(),
                                    this);

    // listen to colord for events
    connect(m_cdInterface, SIGNAL(ProfileAdded(QDBusObjectPath)),
            this, SLOT(profileAdded(QDBusObjectPath)));
    connect(m_cdInterface, SIGNAL(DeviceAdded(QDBusObjectPath)),
            this, SLOT(deviceAdded(QDBusObjectPath)));
    connect(m_cdInterface, SIGNAL(DeviceChanged(QDBusObjectPath)),
            this, SLOT(deviceChanged(QDBusObjectPath)));
}
