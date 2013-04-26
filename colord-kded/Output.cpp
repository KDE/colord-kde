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

#include "Output.h"

#include <QX11Info>

#include <KDebug>

#define RR_CONNECTOR_TYPE_PANEL "Panel"

Output::Output(RROutput output, XRRScreenResources *resources) :
    m_output(output),
    m_resources(resources),
    m_interface(0),
    m_connected(false),
    m_isLaptop(false)
{
    XRROutputInfo *info;
    info = XRRGetOutputInfo(QX11Info::display(), m_resources, m_output);
    if (info == NULL) {
        XRRFreeOutputInfo(info);
        return;
    }

    // store if RROutput is connected
    m_connected = info->connection == RR_Connected;

    // store output name
    m_name = info->name;

    // store the crtc
    m_crtc = info->crtc;

    XRRFreeOutputInfo(info);

    // The ConnectorType property is present in RANDR 1.3 and greater
    if (connectorType() == QLatin1String(RR_CONNECTOR_TYPE_PANEL)) {
        m_isLaptop = true;
    } else if (m_name.contains(QLatin1String("lvds"), Qt::CaseInsensitive) ||
               // Most drivers use an "LVDS" prefix...
               m_name.contains(QLatin1String("LCD"), Qt::CaseInsensitive) ||
               // ... but fglrx uses "LCD" in some versions.  Shoot me now, kthxbye.
               m_name.contains(QLatin1String("eDP"), Qt::CaseInsensitive)
               /* eDP is for internal laptop panel connections */) {
        // Older versions of RANDR - this is a best guess,
        // as @#$% RANDR doesn't have standard output names,
        // so drivers can use whatever they like.
        m_isLaptop = true;
    }
}

Output::~Output()
{
    delete m_interface;
}

bool Output::connected() const
{
    return m_connected;
}

bool Output::isLaptop() const
{
    return m_isLaptop;
}

bool Output::isPrimary(bool hasXRandR13, Window root) const
{
    if (hasXRandR13) {
        RROutput primary = XRRGetOutputPrimary(QX11Info::display(), root);
        return primary == m_output;
    }
    return false;
}

QString Output::name() const
{
    return m_name;
}

QString Output::id() const
{
    return m_id;
}

void Output::setPath(const QDBusObjectPath &path)
{
    if (m_interface && m_interface->path() == path.path()) {
        return;
    }
    m_path = path;

    delete m_interface;
    m_interface = new CdDeviceInterface(QLatin1String("org.freedesktop.ColorManager"),
                                        path.path(),
                                        QDBusConnection::systemBus());
    if (!m_interface->isValid()) {
        kWarning() << "Invalid interface" << path.path() << m_interface->lastError().message();
        delete m_interface;
        m_interface = 0;
    }
}

QDBusObjectPath Output::path() const
{
    return m_path;
}

CdDeviceInterface *Output::interface()
{
    return m_interface;
}

RRCrtc Output::crtc() const
{
    return m_crtc;
}

RROutput Output::output() const
{
    return m_output;
}

int Output::getGammaSize() const
{
    // The gama size of this output
    return XRRGetCrtcGammaSize(QX11Info::display(), m_crtc);
}

void Output::setGamma(XRRCrtcGamma *gamma)
{
    XRRSetCrtcGamma(QX11Info::display(), m_crtc, gamma);
}

Edid Output::readEdidData()
{
    // get the EDID
    size_t size;
    const quint8 *data;
    data = readEdidData(size);
    if (data == NULL) {
        kWarning() << "Unable to get EDID for output" << name();
        Edid ret;
        m_id = ret.deviceId(name());
        return ret;
    }

    // With EDID data we can parse our info
    Edid edid(data, size);
    m_edidHash = edid.hash();
    m_id = edid.deviceId(name());
    delete data;

    return edid;
}

QString Output::edidHash() const
{
    return m_edidHash;
}

QString Output::connectorType() const
{
    unsigned char *prop;
    int actual_format;
    unsigned long nitems, bytes_after;
    Atom actual_type;
    Atom connector_type;
    Atom connector_type_atom = XInternAtom(QX11Info::display(), "ConnectorType", false);
    char *connector_type_str;
    QString result;

    XRRGetOutputProperty(QX11Info::display(), m_output, connector_type_atom,
                         0, 100, false, false,
                         AnyPropertyType,
                         &actual_type, &actual_format,
                         &nitems, &bytes_after, &prop);
    if (!(actual_type == XA_ATOM && actual_format == 32 && nitems == 1)) {
        XFree(prop);
        return result;
    }

    connector_type = *((Atom *) prop);

    connector_type_str = XGetAtomName(QX11Info::display(), connector_type);
    if (connector_type_str) {
        result = connector_type_str;
        XFree(connector_type_str);
    }

    XFree (prop);

    return result;
}

/* This is what gnome-desktop does */
static quint8* getProperty(Display *dpy,
                           RROutput output,
                           Atom atom,
                           size_t &len)
{
    unsigned char *prop;
    int actual_format;
    unsigned long nitems, bytes_after;
    Atom actual_type;
    quint8 *result;

    XRRGetOutputProperty(dpy, output, atom,
                         0, 100, false, false,
                         AnyPropertyType,
                         &actual_type, &actual_format,
                         &nitems, &bytes_after, &prop);
    if (actual_type == XA_INTEGER && actual_format == 8) {
        result = new quint8[nitems];
        memcpy(result, prop, nitems);
        len = nitems;
    } else {
        result = NULL;
    }

    XFree (prop);
    return result;
}

quint8* Output::readEdidData(size_t &len)
{
    Atom edid_atom;
    quint8 *result;

    edid_atom = XInternAtom(QX11Info::display(), RR_PROPERTY_RANDR_EDID, false);
    result = getProperty(QX11Info::display(), m_output, edid_atom, len);

    if (result) {
        if (len % 128 == 0) {
            return result;
        } else {
            len = 0;
            delete result;
        }
    }

    return NULL;
}

bool Output::operator==(const Output &output) const
{
    return m_output == output.output();
}
