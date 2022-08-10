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

#include "ColorD.h"

#include "ProfilesWatcher.h"
#include "XEventHandler.h"
#include "DmiUtils.h"

#include "CdInterface.h"
#include "CdDeviceInterface.h"
#include "CdProfileInterface.h"

#include <lcms2.h>

#include <QApplication>
#include <QFile>
#include <QStringBuilder>
#include <QX11Info>
#include <QLoggingCategory>
#include <QDBusError>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusServiceWatcher>
#include <QDBusUnixFileDescriptor>
#include <QDBusReply>

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(ColorDFactory,
                           "colord.json",
                           registerPlugin<ColorD>();)

Q_LOGGING_CATEGORY(COLORD, "colord")

typedef QList<QDBusObjectPath> ObjectPathList;

ColorD::ColorD(QObject *parent, const QVariantList &) :
    KDEDModule(parent)
{
    if (QApplication::platformName() != QLatin1String("xcb")) {
        // Wayland is not supported
        qCInfo(COLORD, "X11 not detect disabling");
        return;
    }

    // Register this first or the first time will fail
    qRegisterMetaType<CdStringMap>();
    qDBusRegisterMetaType<CdStringMap>();
    qDBusRegisterMetaType<QDBusUnixFileDescriptor>();
    qDBusRegisterMetaType<ObjectPathList>();
    qRegisterMetaType<Edid>();

    // connect to colord using DBus
    connectToColorD();

    // Connect to the display
    if ((m_resources = connectToDisplay()) == nullptr) {
        qCWarning(COLORD) << "Failed to connect to DISPLAY and get the needed resources";
        return;
    }

    // Make sure we know is colord is running
    auto watcher = new QDBusServiceWatcher(QStringLiteral("org.freedesktop.ColorManager"),
                                           QDBusConnection::systemBus(),
                                           QDBusServiceWatcher::WatchForOwnerChange,
                                           this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged,
            this, &ColorD::serviceOwnerChanged);

    // Create the profiles watcher thread
    m_profilesWatcher = new ProfilesWatcher;
    m_profilesWatcher->start();

    // Check outputs add all active outputs, once profiles are ready
    connect(m_profilesWatcher, &ProfilesWatcher::scanFinished,
            this, &ColorD::checkOutputs, Qt::QueuedConnection);

    // init the settings
    init();
}

ColorD::~ColorD()
{
    const auto connectedOutputs = m_connectedOutputs;
    for (const Output::Ptr &out : connectedOutputs) {
        removeOutput(out);
    }

    if (m_x11EventHandler) {
        m_x11EventHandler->deleteLater();
    }

    // Stop the thread
    if (m_profilesWatcher) {
        m_profilesWatcher->quit();
        m_profilesWatcher->wait();
        m_profilesWatcher->deleteLater();
    }
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
    // TODO do it async
    QDBusReply<ObjectPathList> paths = m_cdInterface->GetProfiles();

    // Search through all profiles to see if the edid md5 matches
    foreach (const QDBusObjectPath &profilePath, paths.value()) {
        const CdStringMap metadata = getProfileMetadata(profilePath);
        const auto data = metadata.constFind(QStringLiteral("EDID_md5"));
        if (data != metadata.constEnd() && data.value() == output->edidHash()) {
            qCDebug(COLORD) << "Found EDID profile for device" << profilePath.path() << output->name();
            if (output->interface()) {
                output->interface()->AddProfile(QStringLiteral("soft"), profilePath);
            }
        }
    }
}

CdStringMap ColorD::getProfileMetadata(const QDBusObjectPath &profilePath)
{
    CdProfileInterface profile(QStringLiteral("org.freedesktop.ColorManager"),
                               profilePath.path(),
                               QDBusConnection::systemBus());
    return profile.metadata();
}

void ColorD::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(serviceName)
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
    QString edidVendor = QStringLiteral("unknown");
    QString edidModel = edidVendor;
    QString edidSerial = edidVendor;
    QString deviceId = QStringLiteral("xrandr-unknown");

    // ensure the RROutput is active
    if (!output->isActive()) {
        qCDebug(COLORD) << "output is not active" << output->name();
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
    properties[CD_DEVICE_PROPERTY_KIND] = QStringLiteral("display");
    properties[CD_DEVICE_PROPERTY_MODE] = QStringLiteral("physical");
    properties[CD_DEVICE_PROPERTY_COLORSPACE] = QStringLiteral("rgb");
    properties[CD_DEVICE_PROPERTY_VENDOR] = edidVendor;
    properties[CD_DEVICE_PROPERTY_MODEL] = edidModel;
    properties[CD_DEVICE_PROPERTY_SERIAL] = edidSerial;
    properties[CD_DEVICE_METADATA_XRANDR_NAME] = output->name();
    if (output->isPrimary(m_has_1_3, m_root)) {
        properties[CD_DEVICE_METADATA_OUTPUT_PRIORITY] = CD_DEVICE_METADATA_OUTPUT_PRIORITY_PRIMARY;
    } else {
        properties[CD_DEVICE_METADATA_OUTPUT_PRIORITY] = CD_DEVICE_METADATA_OUTPUT_PRIORITY_PRIMARY;
    }
    properties[CD_DEVICE_METADATA_OUTPUT_EDID_MD5] = output->edidHash();
    properties[CD_DEVICE_PROPERTY_EMBEDDED] = output->isLaptop();

    // We use temp because if we crash or quit the device gets removed
    qCDebug(COLORD) << "Adding device id" << deviceId;
    qCDebug(COLORD) << "Output Hash" << output->edidHash();
    qCDebug(COLORD) << "Output isLaptop" << output->isLaptop();

    // TODO do async
    QDBusReply<QDBusObjectPath> reply;
    reply = m_cdInterface->CreateDevice(deviceId, QStringLiteral("temp"), properties);
    if (reply.isValid()) {
        qCDebug(COLORD) << "Created colord device" << reply.value().path();
        // Store the output path into our Output class
        output->setPath(reply.value());

        // Store the active output into the connected list
        m_connectedOutputs << output;

        // Check if there is any EDID profile to be added
        addEdidProfileToDevice(output);

        // Make sure we set the profile on this device
        outputChanged(output);
    } else {
        qCWarning(COLORD) << "Failed to register device:" << reply.error().message();
        reply = m_cdInterface->FindDeviceById(deviceId);
        if (reply.isValid()) {
            qCDebug(COLORD) << "Found colord device" << reply.value().path();

            bool found = false;
            for (auto iter : m_connectedOutputs) {
                if (iter->id() == deviceId) {
                    found = true;

                    // Store the output path into our Output class
                    iter->setPath(reply.value());

                    // Check if there is any EDID profile to be added
                    addEdidProfileToDevice(iter);

                    // Make sure we set the profile on this device
                    outputChanged(iter);
                    break;
                }
            }

            if (!found) {
                qCDebug(COLORD) << "Failed to locate" << deviceId << "in the list of known outputs";
            }
        }
    }
}

int ColorD::getPrimaryCRTCId(XID primary) const
{
    for (int crtc = 0; crtc < m_resources->ncrtc; crtc++) {
        XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(m_dpy, m_resources, m_resources->crtcs[crtc]);
        if (!crtcInfo) {
            continue;
        }

        if (crtcInfo->mode != None && crtcInfo->noutput > 0) {
            for (int output = 0; output < crtcInfo->noutput; output++) {
                if (crtcInfo->outputs[output] == primary) {
                    return crtc;
                }
            }
        }
        XRRFreeCrtcInfo(crtcInfo);
    }

    return -1;
}

QList<ColorD::X11Monitor> ColorD::getAtomIds() const
{
    QList<ColorD::X11Monitor> monitorList;

    if (!m_resources) {
        return monitorList;
    }

    // see if there is a primary screen.
    const XID primary = XRRGetOutputPrimary(m_dpy, m_root);
    const int primaryId = getPrimaryCRTCId(primary);
    bool havePrimary = false;
    if (primaryId == -1) {
        qCDebug(COLORD) << "Couldn't locate primary CRTC.";
    } else {
        qCDebug(COLORD) << "Primary CRTC is at CRTC " << primaryId;
        havePrimary = true;
    }

    // now iterate over the CRTCs again and add the relevant ones to the list
    int atomId = 0; // the id of the x atom. might be changed when sorting in the primary later!
    for (int crtc = 0; crtc < m_resources->ncrtc; ++crtc) {
        XRROutputInfo *outputInfo = nullptr;
        XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(m_dpy, m_resources, m_resources->crtcs[crtc]);
        if (!crtcInfo) {
            qCDebug(COLORD) << "Can't get CRTC info for CRTC " << crtc;
            continue;
        }
        // only handle those that are attached though
        if (crtcInfo->mode == None || crtcInfo->noutput <= 0) {
            qCDebug(COLORD) << "CRTC for CRTC " << crtc << " has no mode or no output, skipping";
            XRRFreeCrtcInfo(crtcInfo);
            continue;
        }

        // Choose the primary output of the CRTC if we have one, else default to the first. i.e. we punt with
        // mirrored displays.
        bool isPrimary = false;
        int output = 0;
        if (havePrimary) {
            for (int j = 0; j < crtcInfo->noutput; j++) {
                if (crtcInfo->outputs[j] == primary) {
                    output = j;
                    isPrimary = true;
                    break;
                }
            }
        }

        outputInfo = XRRGetOutputInfo(m_dpy, m_resources, crtcInfo->outputs[output]);
        if (!outputInfo) {
            qCDebug(COLORD) << "Can't get output info for CRTC " << crtc << " output " << output;
            XRRFreeCrtcInfo(crtcInfo);
            XRRFreeOutputInfo(outputInfo);
            continue;
        }

        if (outputInfo->connection == RR_Disconnected) {
            qCDebug(COLORD) << "CRTC " << crtc << " output " << output << " is disconnected, skipping";
            XRRFreeCrtcInfo(crtcInfo);
            XRRFreeOutputInfo(outputInfo);
            continue;
        }

        ColorD::X11Monitor monitor;

        monitor.crtc = m_resources->crtcs[crtc];
        monitor.isPrimary = isPrimary;
        monitor.atomId = atomId++;
        monitor.name = outputInfo->name;
        monitorList.append(monitor);

        XRRFreeCrtcInfo(crtcInfo);
        XRRFreeOutputInfo(outputInfo);
    }

    // sort the list of monitors so that the primary one is first. also updates the atomId.
    struct {
        bool operator()(const ColorD::X11Monitor &monitorA,
                        const ColorD::X11Monitor &monitorB) const
        {
            if(monitorA.isPrimary) return true;
            if(monitorB.isPrimary) return false;

            return monitorA.atomId < monitorB.atomId;
        }
    } sortMonitorList;
    std::sort(monitorList.begin(), monitorList.end(), sortMonitorList);
    atomId = 0;
    for (auto monitor : monitorList) {
        monitor.atomId = atomId++;
    }

    return monitorList;
}

void ColorD::outputChanged(const Output::Ptr &output)
{
    qCDebug(COLORD) << "Device changed" << output->path().path();

    if (!output->interface()) {
        return;
    }

    // check Device.Kind is "display"
    if (output->interface()->kind() != QLatin1String("display")) {
        // not a display device, ignoring
        qCDebug(COLORD) << "Not a display device, ignoring" << output->name() << output->interface()->kind();
        return;
    }

    QList<QDBusObjectPath> profiles = output->interface()->profiles();
    if (profiles.isEmpty()) {
        // There are no profiles ignoring
        qCDebug(COLORD) << "There are no profiles, ignoring" << output->name();
        return;
    }

    // read the default profile (the first path in the Device.Profiles property)
    QDBusObjectPath profileDefault = profiles.first();
    qCDebug(COLORD) << "profileDefault" << profileDefault.path();
    CdProfileInterface profile(QStringLiteral("org.freedesktop.ColorManager"),
                               profileDefault.path(),
                               QDBusConnection::systemBus());
    if (!profile.isValid()) {
        qCDebug(COLORD) << "Profile invalid" << output->name() << profile.lastError();
        return;
    }
    QString filename = profile.filename();
    qCDebug(COLORD) << "Default Profile Filename" << output->name() << filename;

    QFile file(filename);
    QByteArray data;
    if (file.open(QIODevice::ReadOnly)) {
        data = file.readAll();
    } else {
        qCWarning(COLORD) << "Failed to open profile" << output->name() << filename;
        return;
    }

    // read the VCGT data using lcms2
    const cmsToneCurve **vcgt;
    cmsHPROFILE lcms_profile = nullptr;

    // open file
    lcms_profile = cmsOpenProfileFromMem((const uint*) data.data(), data.size());
    if (lcms_profile == nullptr) {
        qCWarning(COLORD) << "Could not open profile with lcms" << output->name() << filename;
        return;
    }

    // The gamma size of this output
    int gammaSize = output->getGammaSize();
    if (gammaSize == 0) {
        qCWarning(COLORD) << "Gamma size is zero" << output->name();
        cmsCloseProfile(lcms_profile);
        return;
    }

    // Allocate the gamma
    XRRCrtcGamma *gamma = XRRAllocGamma(gammaSize);

    // get tone curves from profile
    vcgt = static_cast<const cmsToneCurve **>(cmsReadTag(lcms_profile, cmsSigVcgtTag));
    if (vcgt == nullptr || vcgt[0] == nullptr) {
        qCDebug(COLORD) << "Profile does not have any VCGT data, reseting" << output->name() << filename;
        // Reset the gamma table
        for (int i = 0; i < gammaSize; ++i) {
            uint value = (i * 0xffff) / (gammaSize - 1);
            gamma->red[i]   = value;
            gamma->green[i] = value;
            gamma->blue[i]  = value;
        }
    } else {
        // Fill the gamma table with the VCGT data
        for (int i = 0; i < gammaSize; ++i) {
            cmsFloat32Number in;
            in = (double) i / (double) (gammaSize - 1);
            gamma->red[i]   = cmsEvalToneCurveFloat(vcgt[0], in) * (double) 0xffff;
            gamma->green[i] = cmsEvalToneCurveFloat(vcgt[1], in) * (double) 0xffff;
            gamma->blue[i]  = cmsEvalToneCurveFloat(vcgt[2], in) * (double) 0xffff;
        }
    }
    cmsCloseProfile(lcms_profile);

    // push the data to the Xrandr gamma ramps for the display
    output->setGamma(gamma);

    XRRFreeGamma(gamma);

    // export the file data as an x atom
    // during startup the order of outputs can change, so caching the atomId doesn't work that great
    int atomId = -1;
    const QList<ColorD::X11Monitor> monitorList = getAtomIds();
    for (auto monitor : monitorList) {
        if (monitor.crtc == output->crtc()) {
            atomId = monitor.atomId;
            break;
        }
    }
    if (atomId >= 0) {
        QString atomString = QLatin1String("_ICC_PROFILE");
        if (atomId > 0) {
            atomString.append(QString("_%1").arg(atomId));
        }
        qCInfo(COLORD) << "Setting X atom (id:" << atomId << ")" << atomString << "on output:"  << output->name();
        QByteArray atomBytes = atomString.toLatin1();
        const char *atomChars = atomBytes.constData();
        Atom prop = XInternAtom(m_dpy, atomChars, false);
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
            qCWarning(COLORD) << "Failed to set XProperty";
        }
    }
    else {
      qCDebug(COLORD) << "Failed to get an atomId for" << output->name();
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
        qCWarning(COLORD) << "RandR extension missing";
        return nullptr;
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
        qCDebug(COLORD) << "Using XRANDR extension 1.3 or greater.";
    } else if (has_1_2) {
        qCDebug(COLORD) << "Using XRANDR extension 1.2.";
    } else {
        qCDebug(COLORD) << "Using legacy XRANDR extension (1.1 or earlier).";
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
    qCDebug(COLORD) << "Checking outputs";
    // Check the output as something has changed
    for (int i = 0; i < m_resources->noutput; ++i) {
        bool found = false;
        Output::Ptr currentOutput(new Output(m_resources->outputs[i], m_resources));
        foreach (const Output::Ptr &output, m_connectedOutputs) {
            if (output->output() == m_resources->outputs[i]) {
                if (!currentOutput->isActive()) {
                    // The device is not active anymore
                    qCDebug(COLORD) << "remove device";
                    removeOutput(output);
                    found = true;
                    break;
                }
            }
        }

        if (!found && currentOutput->isActive()) {
            // Output is now connected and active
            addOutput(currentOutput);
        }
    }
}

void ColorD::profileAdded(const QDBusObjectPath &profilePath)
{
    // check if the EDID_md5 Profile.Metadata matches any active
    // XRandR devices (e.g. lvds1), otherwise ignore
    const CdStringMap metadata = getProfileMetadata(profilePath);
    const auto data = metadata.constFind(QStringLiteral("EDID_md5"));
    if (data != metadata.constEnd()) {
        const QString edidHash = data.value();
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
            output->interface()->AddProfile(QStringLiteral("soft"), profilePath);
        }
    }
}

void ColorD::deviceAdded(const QDBusObjectPath &objectPath)
{
    qCDebug(COLORD) << "Device added" << objectPath.path();
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
    qCDebug(COLORD) << "Device changed" << objectPath.path();
    Output::Ptr output;
    // Get the Crtc of this output
    for (int i = 0; i < m_connectedOutputs.size(); ++i) {
        if (m_connectedOutputs.at(i)->path() == objectPath) {
            output = m_connectedOutputs[i];
            break;
        }
    }

    if (output.isNull()) {
        qCWarning(COLORD) << "Output not found";
        return;
    }

    outputChanged(output);
}

void ColorD::connectToColorD()
{
    // Creates a ColorD interface, it must be created with new
    // otherwise the object will be deleted when this block ends
    m_cdInterface = new CdInterface(QStringLiteral("org.freedesktop.ColorManager"),
                                    QStringLiteral("/org/freedesktop/ColorManager"),
                                    QDBusConnection::systemBus(),
                                    this);

    // listen to colord for events
    connect(m_cdInterface, &CdInterface::ProfileAdded,
            this, &ColorD::profileAdded);
    connect(m_cdInterface, &CdInterface::DeviceAdded,
            this, &ColorD::deviceAdded);
    connect(m_cdInterface, &CdInterface::DeviceChanged,
            this, &ColorD::deviceChanged);
}

#include "ColorD.moc"
