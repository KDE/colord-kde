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

#ifndef OUTPUT_H
#define OUTPUT_H

#include <QDBusObjectPath>
#include <QTextStream>

#include "Edid.h"

extern "C"
{
    #include <X11/Xlib.h>
    #define INT8 _X11INT8
    #define INT32 _X11INT32
    #include <X11/Xproto.h>
    #undef INT8
    #undef INT32
    #include <X11/extensions/Xrandr.h>
}

class Output
{
    Q_GADGET
public:
    Output(RROutput output, XRRScreenResources *resources);

    bool connected() const;
    bool isLaptop() const;
    bool isPrimary(bool hasXRandR13, Window root) const;
    QString name() const;
    void setPath(const QDBusObjectPath &path);
    QDBusObjectPath path() const;
    RRCrtc crtc() const;
    RROutput output() const;
    int getGammaSize();
    void setGamma(XRRCrtcGamma *gamma);

    Edid readEdidData();
    QString edidHash() const;

    bool operator==(const Output &output) const;

private:
    /**
      * Callers should delete the data if not 0
      */
    quint8* readEdidData(size_t &len);

    RROutput m_output;
    XRRScreenResources *m_resources;
    QString m_edidHash;
    QDBusObjectPath m_path;

    bool m_connected;
    QString m_name;
    RRCrtc m_crtc;
};

#endif // OUTPUT_H
