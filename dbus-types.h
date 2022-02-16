/***************************************************************************
 *   Copyright (C) 2013 by Daniel Nicoletti <dantti12@gmail.com>           *
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

#ifndef DBUS_TYPES_H
#define DBUS_TYPES_H

#include <QMap>
#include <QMetaType>

typedef QMap<QString, QString> CdStringMap;
Q_DECLARE_METATYPE(CdStringMap)

struct Gamma {
    double red;
    double green;
    double blue;
};
typedef QList<Gamma> CdGamaList;
Q_DECLARE_METATYPE(CdGamaList)

/* defined in org.freedesktop.ColorManager.Device.xml */
#define CD_DEVICE_PROPERTY_MODEL QStringLiteral("Model")
#define CD_DEVICE_PROPERTY_KIND QStringLiteral("Kind")
#define CD_DEVICE_PROPERTY_VENDOR QStringLiteral("Vendor")
#define CD_DEVICE_PROPERTY_SERIAL QStringLiteral("Serial")
#define CD_DEVICE_PROPERTY_COLORSPACE QStringLiteral("Colorspace")
#define CD_DEVICE_PROPERTY_FORMAT QStringLiteral("Format")
#define CD_DEVICE_PROPERTY_MODE QStringLiteral("Mode")
#define CD_DEVICE_PROPERTY_PROFILES QStringLiteral("Profiles")
#define CD_DEVICE_PROPERTY_CREATED QStringLiteral("Created")
#define CD_DEVICE_PROPERTY_MODIFIED QStringLiteral("Modified")
#define CD_DEVICE_PROPERTY_METADATA QStringLiteral("Metadata")
#define CD_DEVICE_PROPERTY_ID QStringLiteral("DeviceId")
#define CD_DEVICE_PROPERTY_SCOPE QStringLiteral("Scope")
#define CD_DEVICE_PROPERTY_OWNER QStringLiteral("Owner")
#define CD_DEVICE_PROPERTY_SEAT QStringLiteral("Seat")
#define CD_DEVICE_PROPERTY_PROFILING_INHIBITORS QStringLiteral("ProfilingInhibitors")
#define CD_DEVICE_PROPERTY_ENABLED QStringLiteral("Enabled")
#define CD_DEVICE_PROPERTY_EMBEDDED QStringLiteral("Embedded")

/* defined in metadata-spec.txt */
#define CD_DEVICE_METADATA_XRANDR_NAME QStringLiteral("XRANDR_name")
#define CD_DEVICE_METADATA_OUTPUT_EDID_MD5 QStringLiteral("OutputEdidMd5")
#define CD_DEVICE_METADATA_OUTPUT_PRIORITY QStringLiteral("OutputPriority")
#define CD_DEVICE_METADATA_OUTPUT_PRIORITY_PRIMARY QStringLiteral("primary")
#define CD_DEVICE_METADATA_OUTPUT_PRIORITY_SECONDARY QStringLiteral("secondary")
#define CD_DEVICE_METADATA_OWNER_CMDLINE QStringLiteral("OwnerCmdline")

#endif
