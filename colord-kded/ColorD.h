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
class ColorD : public KDEDModule
{
    Q_OBJECT
public:
    ColorD(QObject *parent, const QVariantList &args);
    ~ColorD();

private slots:
    void profileAdded(const QDBusObjectPath &objectPath);
    void deviceAdded(const QDBusObjectPath &objectPath);
    void deviceChanged(const QDBusObjectPath &objectPath);

    void addProfile(const QString &filename);
    void removeProfile(const QString &filename);

private:
    quint8* readEdidData(RROutput output, size_t &len);
    void scanHomeDirectory();
    void connectToDisplay();
    void connectToColorD();
    void addOutput(RROutput output);
    void removeOutput(RROutput output);
    void addProfile(const QFileInfo &fileInfo);
    bool outputIsLaptop(RROutput output, const QString &outputName) const;
    QString dmiGetName() const;
    QString dmiGetVendor() const;

    QHash<QString, Edid> m_edids;
    QHash<QString, QDBusObjectPath> m_devices;
    Display *m_dpy;
    XRRScreenResources *m_resources;
    Window m_root;
    
    bool m_valid;
    QString m_errorCode;
    QString m_version;

    int m_eventBase;
    int m_errorBase;
};

Q_DECLARE_METATYPE(StringStringMap)

#endif // COLORD_H
