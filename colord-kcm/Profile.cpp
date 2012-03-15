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

#include "Profile.h"

#include <KLocale>
#include <KFile>
#include <KDebug>

#define CD_PROFILE_METADATA_DATA_SOURCE_EDID	 "edid"
#define CD_PROFILE_METADATA_DATA_SOURCE_CALIB	 "calib"
#define CD_PROFILE_METADATA_DATA_SOURCE_STANDARD "standard"
#define CD_PROFILE_METADATA_DATA_SOURCE_TEST     "test"

Profile::Profile(const QString &filename)
{
    setFilename(filename);
}

void Profile::setFilename(const QString &filename)
{
    if (!filename.isEmpty()) {
        m_filename = filename;
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data;
            data = file.readAll();
            parseProfile((const uint*) data.data(), data.size());
        }
    }
}

void Profile::parseProfile(const uint *data, size_t length)
{
    /* save the length */
    m_size = length;
    m_loaded = true;

    /* ensure we have the header */
    if (length < 0x84) {
        kWarning() << "profile was not valid (file size too small)";
        return;
//        g_set_error (error, 1, 0, "profile was not valid (file size too small)");
//        goto out;
    }

    /* load profile into lcms */
    m_lcmsProfile = cmsOpenProfileFromMem(data, length);
    if (m_lcmsProfile == NULL) {
        kWarning() << "failed to load: not an ICC profile";
        return;
//        g_set_error_literal (error, 1, 0, "failed to load: not an ICC profile");
//        goto out;
    }

    /* get white point */
    cmsCIEXYZ *cie_xyz;
    bool ret;
    cie_xyz = static_cast<cmsCIEXYZ*>(cmsReadTag(m_lcmsProfile, cmsSigMediaWhitePointTag));
    if (cie_xyz != NULL) {
        cmsCIExyY xyY;
        double temp_float;
        m_white.setX(cie_xyz->X);
        m_white.setY(cie_xyz->Y);
        m_white.setZ(cie_xyz->Z);
//        cd_color_set_xyz (priv->white,
//                          cie_xyz->X, cie_xyz->Y, cie_xyz->Z);

        /* convert to lcms xyY values */
        cmsXYZ2xyY(&xyY, cie_xyz);
        kDebug() << "whitepoint:" << xyY.x << xyY.y << xyY.Y;
//        g_debug ("whitepoint = %f,%f [%f]", xyY.x, xyY.y, xyY.Y);

        /* get temperature */
        ret = cmsTempFromWhitePoint(&temp_float, &xyY);
        if (ret) {
            /* round to nearest 100K */
            m_temperature = (((uint) temp_float) / 100) * 100;
            kDebug() << "color temperature:" << m_temperature;
//            g_debug ("color temperature = %i", priv->temperature);
        } else {
            m_temperature = 0;
            kWarning() << "failed to get color temperature";
//            priv->temperature = 0;
//            g_warning ("failed to get color temperature");
        }
    } else {
        /* this is no big suprise, some profiles don't have these */
        m_white.setX(0);
        m_white.setY(0);
        m_white.setZ(0);
        kDebug() << "failed to get white point";
//        cd_color_clear_xyz (priv->white);
//        g_debug ("failed to get white point");
    }

    /* get the profile kind */
    cmsProfileClassSignature profile_class;
    profile_class = cmsGetDeviceClass(m_lcmsProfile);
    switch (profile_class) {
    case cmsSigInputClass:
        m_kind = KindInputDevice;
        break;
    case cmsSigDisplayClass:
        m_kind = KindDisplayDevice;
        break;
    case cmsSigOutputClass:
        m_kind = KindOutputDevice;
        break;
    case cmsSigLinkClass:
        m_kind = KindDeviceLink;
        break;
    case cmsSigColorSpaceClass:
        m_kind = KindColorspaceConversion;
        break;
    case cmsSigAbstractClass:
        m_kind = KindAbstract;
        break;
    case cmsSigNamedColorClass:
        m_kind = KindNamedColor;
        break;
    default:
        m_kind = KindUnknown;
    }

    /* get colorspace */
    cmsColorSpaceSignature color_space;
    color_space = cmsGetColorSpace(m_lcmsProfile);
    switch (color_space) {
    case cmsSigXYZData:
        m_colorspace = i18n("XYZ");
        break;
    case cmsSigLabData:
        m_colorspace = i18n("LAB");
        break;
    case cmsSigLuvData:
        m_colorspace = i18n("LUV");
        break;
    case cmsSigYCbCrData:
        m_colorspace = i18n("YCbCr");
        break;
    case cmsSigYxyData:
        m_colorspace = i18n("Yxy");
        break;
    case cmsSigRgbData:
        m_colorspace = i18n("RGB");
        break;
    case cmsSigGrayData:
        m_colorspace = i18n("Gray");
        break;
    case cmsSigHsvData:
        m_colorspace = i18n("HSV");
        break;
    case cmsSigCmykData:
        m_colorspace = i18n("CMYK");
        break;
    case cmsSigCmyData:
        m_colorspace = i18n("CMY");
        break;
    default:
        m_colorspace = i18n("Unknown");
    }

    /* get the illuminants from the primaries */
//    if (color_space == cmsSigRgbData) {
//        cie_xyz = cmsReadTag (m_lcmsProfile, cmsSigRedMatrixColumnTag);
//        if (cie_xyz != NULL) {
//            /* assume that if red is present, the green and blue are too */
//            cd_color_copy_xyz ((CdColorXYZ *) cie_xyz, (CdColorXYZ *) &cie_illum.Red);
//            cie_xyz = cmsReadTag (m_lcmsProfile, cmsSigGreenMatrixColumnTag);
//            cd_color_copy_xyz ((CdColorXYZ *) cie_xyz, (CdColorXYZ *) &cie_illum.Green);
//            cie_xyz = cmsReadTag (m_lcmsProfile, cmsSigBlueMatrixColumnTag);
//            cd_color_copy_xyz ((CdColorXYZ *) cie_xyz, (CdColorXYZ *) &cie_illum.Blue);
//            got_illuminants = TRUE;
//        } else {
//            g_debug ("failed to get illuminants");
//        }
//    }

    /* get the illuminants by running it through the profile */
//    if (!got_illuminants && color_space == cmsSigRgbData) {
//        gdouble rgb_values[3];

//        /* create a transform from profile to XYZ */
//        xyz_profile = cmsCreateXYZProfile ();
//        transform = cmsCreateTransform (m_lcmsProfile, TYPE_RGB_DBL, xyz_profile, TYPE_XYZ_DBL, INTENT_PERCEPTUAL, 0);
//        if (transform != NULL) {

//            /* red */
//            rgb_values[0] = 1.0;
//            rgb_values[1] = 0.0;
//            rgb_values[2] = 0.0;
//            cmsDoTransform (transform, rgb_values, &cie_illum.Red, 1);

//            /* green */
//            rgb_values[0] = 0.0;
//            rgb_values[1] = 1.0;
//            rgb_values[2] = 0.0;
//            cmsDoTransform (transform, rgb_values, &cie_illum.Green, 1);

//            /* blue */
//            rgb_values[0] = 0.0;
//            rgb_values[1] = 0.0;
//            rgb_values[2] = 1.0;
//            cmsDoTransform (transform, rgb_values, &cie_illum.Blue, 1);

//            /* we're done */
//            cmsDeleteTransform (transform);
//            got_illuminants = TRUE;
//        } else {
//            g_debug ("failed to run through profile");
//        }

//        /* no more need for the output profile */
//        cmsCloseProfile (xyz_profile);
//    }

    /* we've got valid values */
//    if (got_illuminants) {
//        cd_color_set_xyz (priv->red,
//                          cie_illum.Red.X, cie_illum.Red.Y, cie_illum.Red.Z);
//        cd_color_set_xyz (priv->green,
//                          cie_illum.Green.X, cie_illum.Green.Y, cie_illum.Green.Z);
//        cd_color_set_xyz (priv->blue,
//                          cie_illum.Blue.X, cie_illum.Blue.Y, cie_illum.Blue.Z);
//    } else {
//        g_debug ("failed to get luminance values");
//        cd_color_clear_xyz (priv->red);
//        cd_color_clear_xyz (priv->green);
//        cd_color_clear_xyz (priv->blue);
//    }

    /* get the profile created time and date */
    struct tm created;
    ret = cmsGetHeaderCreationDateTime(m_lcmsProfile, &created);
    if (ret) {
        m_datetime = parseDateTime(created);
    }

    /* get profile header version */
    cmsFloat64Number profile_version;
    profile_version = cmsGetProfileVersion(m_lcmsProfile);
    m_version = QString::number(profile_version, 'f', 2);

    /* allocate temporary buffer */
    char *text = new char[1024];

    /* get description */
    ret = cmsGetProfileInfoASCII(m_lcmsProfile, cmsInfoDescription, "en", "US", text, 1024);
    if (ret) {
        m_description = QString::fromAscii(text).simplified();
        if (m_description.isEmpty()) {
            m_description = i18n("Missing description");
        }
    }

    /* get copyright */
    ret = cmsGetProfileInfoASCII(m_lcmsProfile, cmsInfoCopyright, "en", "US", text, 1024);
    if (ret) {
        m_copyright = QString::fromAscii(text).simplified();
    }

    /* get description */
    ret = cmsGetProfileInfoASCII(m_lcmsProfile, cmsInfoManufacturer, "en", "US", text, 1024);
    if (ret) {
        m_manufacturer = QString::fromAscii(text).simplified();

    }

    /* get description */
    ret = cmsGetProfileInfoASCII(m_lcmsProfile, cmsInfoModel, "en", "US", text, 1024);
    if (ret) {
        m_model = QString::fromAscii(text).simplified();
    }

    delete [] text;

//    /* success */
//    ret = TRUE;

//    /* generate and set checksum */
//    checksum = g_compute_checksum_for_data (G_CHECKSUM_MD5, (const guchar *) data, length);
//    gcm_profile_set_checksum (profile, checksum);
//out:
//    g_free (text);
//    g_free (checksum);
//    return ret;
}

QDateTime Profile::parseDateTime(const struct tm &created)
{
    QDateTime ret;
    QTime time;
    time.setHMS(created.tm_hour, created.tm_min, created.tm_sec);
    ret.setTime(time);
    QDate date;
    date.setYMD(created.tm_year, created.tm_mon + 1, created.tm_mday);
    ret.setDate(date);

    return ret;
}

bool Profile::loaded() const
{
    return m_loaded;
}

Profile::ProfileKind Profile::kind() const
{
    return m_kind;
}

QString Profile::kindString() const
{
    switch (kind()) {
    case KindInputDevice:
        return i18n("Input device");
    case KindDisplayDevice:
        return i18n("Display device");
    case KindOutputDevice:
        return i18n("Output device");
    case KindDeviceLink:
        return i18n("Devicelink");
    case KindColorspaceConversion:
        return i18n("Colorspace conversion");
    case KindAbstract:
        return i18n("Abstract");
    case KindNamedColor:
        return i18n("Named color");
    default:
        return i18n("Unknown");
    }
}

QString Profile::colorspace() const
{
    return m_colorspace;
}

uint Profile::size() const
{
    return m_size;
}

bool Profile::canDelete() const
{
    return m_canDelete;
}

QString Profile::description() const
{
    return m_description;
}

QString Profile::filename() const
{
    return m_filename;
}

QString Profile::version() const
{
    return m_version;
}

QString Profile::copyright() const
{
    return m_copyright;
}

QString Profile::manufacturer() const
{
    return m_manufacturer;
}

QString Profile::model() const
{
    return m_model;
}

QDateTime Profile::datetime() const
{
    return m_datetime;
}

QString Profile::checksum() const
{
    return m_checksum;
}

uint Profile::temperature() const
{
    return m_temperature;
}

QString Profile::profileWithSource(const QString &dataSource, const QString &profilename)
{
    if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_EDID)) {
        return i18n("Default: %1", profilename);
    } else if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_STANDARD)) {
        return i18n("Colorspace: %1", profilename);
    } if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_TEST)) {
        return i18n("Test profile: %1", profilename);
    }
    return profilename;
}
