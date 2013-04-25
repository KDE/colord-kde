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

#ifndef COLORD_H
#define COLORD_H

#include "Output.h"
#include "../dbus-types.h"

#include <KDEDModule>

extern "C"
{
    #include <fixx11h.h>
    #include <X11/Xatom.h>
    #include <X11/extensions/Xrandr.h>
}

class CdInterface;
class XEventHandler;
class ProfilesWatcher;
class ColorD : public KDEDModule
{
    Q_OBJECT
public:
    ColorD(QObject *parent, const QVariantList &args);
    ~ColorD();
    void reset();
    int eventBase() const;
    bool isValid() const;

private slots:
    void init();
    void checkOutputs();

    void profileAdded(const QDBusObjectPath &objectPath);
    void deviceAdded(const QDBusObjectPath &objectPath);
    void deviceChanged(const QDBusObjectPath &objectPath);

    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);

private:
    quint8* readEdidData(RROutput output, size_t &len);
    XRRScreenResources *connectToDisplay();
    void connectToColorD();
    void addOutput(const Output::Ptr &output);
    void outputChanged(const Output::Ptr &output);
    void removeOutput(const Output::Ptr &output);
    void addEdidProfileToDevice(const Output::Ptr &output);
    CdStringMap getProfileMetadata(const QDBusObjectPath &profilePath);
    QString profilesPath() const;

    Output::List m_connectedOutputs;

    Display *m_dpy;
    XRRScreenResources *m_resources;
    Window m_root;

    QString m_errorCode;

    bool m_has_1_3;
    int m_errorBase;
    XEventHandler *m_x11EventHandler;
    ProfilesWatcher *m_profilesWatcher;
    CdInterface *m_cdInterface;
};

#endif // COLORD_H
