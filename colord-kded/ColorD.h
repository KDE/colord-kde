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

#include <KDEDModule>

#include "Output.h"

#include <KDirWatch>

#include <QVariantList>
#include <QFileInfo>
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
typedef QMap<QString, QString>  StringStringMap;

class Edid;
class XEventHandler;
class ColorD : public KDEDModule
{
    Q_OBJECT
public:
    ColorD(QObject *parent, const QVariantList &args);
    ~ColorD();

    void scanHomeDirectory();
    void reset();

private slots:
    void init();
    void checkOutputs();

    void profileAdded(const QDBusObjectPath &objectPath);
    void deviceAdded(const QDBusObjectPath &objectPath);
    void deviceChanged(const QDBusObjectPath &objectPath);

    void addProfile(const QString &filename);
    void removeProfile(const QString &filename);

    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);

private:
    quint8* readEdidData(RROutput output, size_t &len);
    void connectToDisplay();
    void connectToColorD();
    void addOutput(Output &output);
    void outputChanged(Output &output);
    void removeOutput(const Output &output);
    void addProfile(const QFileInfo &fileInfo);
    void addProfileToDevice(const QDBusObjectPath &profilePath, const QDBusObjectPath &devicePath);
    void addEdidProfileToDevice(Output &output);
    StringStringMap getProfileMetadata(const QDBusObjectPath &profilePath);
    QString profilesPath() const;

    KDirWatch *m_dirWatch;
    QList<Output> m_connectedOutputs;

    Display *m_dpy;
    XEventHandler *m_eventHandler;
    XRRScreenResources *m_resources;
    Window m_root;
    
    bool m_valid;
    QString m_errorCode;
    QString m_version;

    bool m_has_1_3;
    int m_eventBase;
    int m_errorBase;
};

Q_DECLARE_METATYPE(StringStringMap)

#endif // COLORD_H
