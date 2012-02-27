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

#ifndef COLORD_H
#define COLORD_H

#include <KDEDModule>
#include <QVariantList>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <fixx11h.h>


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
typedef QList<int> ScreenList;

class ColorD : public KDEDModule
{
    Q_OBJECT
public:
    ColorD(QObject *parent, const QVariantList &args);
    ~ColorD();


private:
    quint8* readEdidData(RROutput output, size_t *len);
    void ScanHomeDirectory();
    void ConnectToDisplay();
    void ConnectToColorD();
    void AddOutput(RROutput output);
    void RemoveOutput(RROutput output);
    void AddProfile(QString filename);
    void profileAdded(QString *object_path);
    void deviceAdded(QString *object_path);
    void deviceChanged(QString *object_path);

    Display *m_dpy;
    XRRScreenResources *m_resources;
    Window m_root;
    
    bool m_valid;
    QString m_errorCode;
    QString m_version;

    int     m_eventBase;
    int m_errorBase;
};

#endif // COLORD_H
