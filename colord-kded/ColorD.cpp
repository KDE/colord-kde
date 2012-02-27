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

#include "ColorD.h"

#include <KLocale>
#include <KGenericFactory>
#include <KNotification>
#include <KIcon>

#include <QX11Info>

K_PLUGIN_FACTORY(ColorDFactory, registerPlugin<ColorD>();)
K_EXPORT_PLUGIN(ColorDFactory("colord"))

bool has_1_2 = false;
bool has_1_3 = false;

ColorD::ColorD(QObject *parent, const QVariantList &args) :
    KDEDModule(parent)
{
    // There's not much use for args in a KCM
    Q_UNUSED(args)

    /* connect to colord using DBus */
    ConnectToColorD();

    /* Connect to the display */
    ConnectToDisplay();

    /* Scan all the *.icc files */
    ScanHomeDirectory();
}

ColorD::~ColorD()
{
}

/* This is what gnome-desktop does */
static quint8* getProperty(Display *dpy,
                           RROutput output,
                           Atom atom,
                           size_t *len)
{
    unsigned char *prop;
    int actual_format;
    unsigned long nitems, bytes_after;
    Atom actual_type;
    quint8 *result;

    XRRGetOutputProperty (dpy, output, atom,
                          0, 100, False, False,
                          AnyPropertyType,
                          &actual_type, &actual_format,
                          &nitems, &bytes_after, &prop);
    if (actual_type == XA_INTEGER && actual_format == 8) {
        result = new quint8[nitems];
        memcpy(result, prop, nitems);
//        g_memdup (prop, nitems);
//        quint8
        if (len)
            *len = nitems;
    } else {
        result = NULL;
    }

    XFree (prop);
    return result;
}

quint8* ColorD::readEdidData(RROutput output, size_t *len)
{
    Atom edid_atom;
    quint8 *result;

    edid_atom = XInternAtom(m_dpy, RR_PROPERTY_RANDR_EDID, FALSE);
    result = getProperty(m_dpy, output, edid_atom, len);
    if (result == NULL) {
        edid_atom = XInternAtom(m_dpy, "EDID_DATA", FALSE);
        result = getProperty (m_dpy, output, edid_atom, len);
    }

    if (result) {
        if (*len % 128 == 0) {
            return result;
        } else {
            delete result;
        }
    }

    return NULL;
}

void ColorD::ScanHomeDirectory()
{
    /* Get a list of files in ~/.local/share/icc/ */
    //TODO

    /* For each file, get the MD5 hash and construct a profile-id
     * according to the device-and-profile-naming-spec.txt */
    //TODO

    /* Call CreateProfile() for each file */
    //TODO
}

void ColorD::AddOutput(RROutput output)
{
    /* ensure the RROutput is connected */
    XRROutputInfo *info;
    info = XRRGetOutputInfo(m_dpy, m_resources, output);
    if (info == NULL) {
        return;
    }
    if (info->connection != RR_Connected) {
        return;
    }

    /* get the EDID */
    //read_edid_data(m_resources->outputs[i])

    /* get the md5 of the EDID blob */
    //TODO

    /* parse the edid and save in a hash table [m_hash_edid_md5?]*/
    //TODO

    /* call CreateDevice() with a device_id constructed using the naming
     * spec: https://gitorious.org/colord/master/blobs/master/doc/device-and-profile-naming-spec.txt */

    /* set the "XRANDR_name" option to the correct xrandr name */
    //TODO: use info->name

    XRRFreeOutputInfo(info);
}


void ColorD::RemoveOutput(RROutput output)
{
  /* find the device in colord using FindDeviceByProperty(info->name) */
  //TODO

  /* call DBus DeleteDevice() on the result */
  //TODO
}

void ColorD::ConnectToDisplay(void)
{
    m_dpy = QX11Info::display();

    // Check extension
    if (XRRQueryExtension(m_dpy, &m_eventBase, &m_errorBase) == False) {
        m_valid = false;
        return;
    }

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

    if (has_1_3)
        m_resources = XRRGetScreenResourcesCurrent(m_dpy, m_root);
    else
        m_resources = XRRGetScreenResources(m_dpy, m_root);

    for (int i = 0; i < m_resources->noutput; ++i) {
        kDebug() << "Adding" << m_resources->outputs[i];
        AddOutput(m_resources->outputs[i]);
    }

    /* register for root window changes */
    XRRSelectInput (m_dpy,
                    m_root,
                    RRScreenChangeNotifyMask);
//    gdk_x11_register_standard_event_type (m_dpy,
//                              m_eventBase,
//                              RRNotify + 1);
//    gdk_window_add_filter (priv->gdk_root, screen_on_event, self);

    /* if monitors are added, call AddOutput() */
    //TODO

    /* if monitors are removed, call RemoveOutput() */
    //TODO
}

void ColorD::ConnectToColorD()
{
    /*
     * Listen to colord for the DBus ::ProfileAdded signal
     *
     *  -  Check if the EDID_md5 Profile.Metadata matches any connected
     * XRandR devices (e.g. lvds1) . If it does, then it needs to call
     * Device.AddProfile() with the device object path that matches and the
     * profile object path. This makes sure that the profile is attached to
     * the right device if you've switched from GNOME to KDE or vice-versa.
     */

    /*
     * Listen to the DBus ::DeviceAdded signal
     *
     * If it is, show a notification that the user should calibrate the
     * device. In GNOME we disable this by default, as it's only really
     * useful in color critical environments like in the animation studios.
     */

    /* Listen to the DBus ::DeviceChanged signal
     *
     *  - If the device is a Xrandr device (i.e. Device.Kind is "display")
     * then read the default profile (the first path in the Device.Profiles
     * property) and open the physical file. From that, read the VCGT data
     * (just copy the code in GCM for this, it uses lcms2) and push the data
     * to the Xrandr gamma ramps for the display. I can show you this code in
     * gnome-desktop if you need to copy, although there's probably a more
     * Solid way of doing this.
     *    - Also load in the entire icc file, and export it as an x atom on
     * the *screen* (not output) called _ICC_PROFILE
     */
}
