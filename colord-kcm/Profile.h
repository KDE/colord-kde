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

#ifndef PROFILE_H
#define PROFILE_H

#include <QString>
#include <QDateTime>
#include <QColor>
#include <QQuaternion>

#include <lcms2.h>

class Profile
{
public:
    typedef enum {
        KindUnknown,
        KindInputDevice,
        KindDisplayDevice,
        KindOutputDevice,
        KindDeviceLink,
        KindColorspaceConversion,
        KindAbstract,
        KindNamedColor
    } ProfileKind;
    Profile(const QString &filename = QString());

    void setFilename(const QString &filename);

    bool loaded() const;
    ProfileKind kind() const;
    QString kindString() const;
    QString colorspace() const;
    uint size() const;
    bool canDelete() const;
    QString description() const;
    QString filename() const;
    QString version() const;
    QString copyright() const;
    QString manufacturer() const;
    QString model() const;
    QDateTime datetime() const;
    QString checksum() const;
    uint temperature() const;

    QMap<QString, QColor> getNamedColors();

    static QString profileWithSource(const QString &dataSource, const QString &profilename);

private:
    QColor convertXYZ(cmsCIEXYZ *cieXYZ);
    void parseProfile(const uint *data, size_t length);
    QDateTime parseDateTime(const struct tm &created);

    bool m_loaded;
    ProfileKind m_kind;
    QString m_colorspace;
    uint m_size;
    bool m_canDelete;
    QString m_description;
    QString m_filename;
    QString m_version;
    QString m_copyright;
    QString m_manufacturer;
    QString m_model;
    QDateTime m_datetime;
    QString m_checksum;
    uint m_temperature;
//    GHashTable		*dict;
//    CdColorXYZ		*white;
    QQuaternion m_white;
//    CdColorXYZ		*red;
//    CdColorXYZ		*green;
//    CdColorXYZ		*blue;
//    GFile			*file;
//    GFileMonitor		*monitor;
//    gboolean		 has_mlut;
    cmsHPROFILE m_lcmsProfile;
};

#endif // PROFILE_H
