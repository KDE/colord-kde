/***************************************************************************
 *   Copyright (C) 2012-2016 by Daniel Nicoletti <dantti12@gmail.com>      *
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

#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QLocale>

#include <KLocalizedString>

Profile::Profile(const QString &filename)
{
    setFilename(filename);
}

Profile::~Profile()
{
    if (m_lcmsProfile) {
        cmsCloseProfile(m_lcmsProfile);
    }
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

QString Profile::errorMessage() const
{
    return m_errorMessage;
}

QColor Profile::convertXYZ(cmsCIEXYZ *cieXYZ)
{
    typedef struct {
        quint8	 R;
        quint8	 G;
        quint8	 B;
    } CdColorRGB8;
    QColor ret;
    CdColorRGB8 rgb;
    cmsHPROFILE profile_srgb = nullptr;
    cmsHPROFILE profile_xyz = nullptr;
    cmsHTRANSFORM xform = nullptr;

    /* nothing set yet */
    if (cieXYZ == nullptr) {
        return ret;
    }

    /* convert the color to sRGB */
    profile_xyz = cmsCreateXYZProfile();
    profile_srgb = cmsCreate_sRGBProfile();
    xform = cmsCreateTransform(profile_xyz, TYPE_XYZ_DBL,
                               profile_srgb, TYPE_RGB_8,
                               INTENT_ABSOLUTE_COLORIMETRIC, 0);
    cmsDoTransform(xform, cieXYZ, &rgb, 1);

    ret.setRgb(rgb.R, rgb.G, rgb.B);

    if (profile_srgb != nullptr) {
        cmsCloseProfile(profile_srgb);
    }
    if (profile_xyz != nullptr) {
        cmsCloseProfile(profile_xyz);
    }
    if (xform != nullptr) {
        cmsDeleteTransform(xform);
    }
    return ret;
}

void Profile::parseProfile(const uint *data, size_t length)
{
    /* save the length */
    m_size = length;
    m_loaded = true;

    /* ensure we have the header */
    if (length < 0x84) {
        qWarning() << "profile was not valid (file size too small)";
        m_errorMessage = i18n("file too small");
        m_loaded = false;
        return;
    }

    /* load profile into lcms */
    m_lcmsProfile = cmsOpenProfileFromMem(data, length);
    if (m_lcmsProfile == nullptr) {
        qWarning() << "failed to load: not an ICC profile";
        m_errorMessage = i18n("not an ICC profile");
        m_loaded = false;
        return;
    }

    /* get white point */
    cmsCIEXYZ *cie_xyz;
    bool ret;
    cie_xyz = static_cast<cmsCIEXYZ*>(cmsReadTag(m_lcmsProfile, cmsSigMediaWhitePointTag));
    if (cie_xyz != nullptr) {
        cmsCIExyY xyY;
        double temp_float;
        m_white.setX(cie_xyz->X);
        m_white.setY(cie_xyz->Y);
        m_white.setZ(cie_xyz->Z);

        /* convert to lcms xyY values */
        cmsXYZ2xyY(&xyY, cie_xyz);
        qDebug() << "whitepoint:" << xyY.x << xyY.y << xyY.Y;

        /* get temperature */
        ret = cmsTempFromWhitePoint(&temp_float, &xyY);
        if (ret) {
            /* round to nearest 100K */
            m_temperature = (((uint) temp_float) / 100) * 100;
            qDebug() << "color temperature:" << m_temperature;
        } else {
            m_temperature = 0;
            qWarning() << "failed to get color temperature";
        }
    } else {
        /* this is no big surprise, some profiles don't have these */
        m_white.setX(0);
        m_white.setY(0);
        m_white.setZ(0);
        qDebug() << "failed to get white point";
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
        m_colorspace = i18nc("colorspace", "XYZ");
        break;
    case cmsSigLabData:
        m_colorspace = i18nc("colorspace", "LAB");
        break;
    case cmsSigLuvData:
        m_colorspace = i18nc("colorspace", "LUV");
        break;
    case cmsSigYCbCrData:
        m_colorspace = i18nc("colorspace", "YCbCr");
        break;
    case cmsSigYxyData:
        m_colorspace = i18nc("colorspace", "Yxy");
        break;
    case cmsSigRgbData:
        m_colorspace = i18nc("colorspace", "RGB");
        break;
    case cmsSigGrayData:
        m_colorspace = i18nc("colorspace", "Gray");
        break;
    case cmsSigHsvData:
        m_colorspace = i18nc("colorspace", "HSV");
        break;
    case cmsSigCmykData:
        m_colorspace = i18nc("colorspace", "CMYK");
        break;
    case cmsSigCmyData:
        m_colorspace = i18nc("colorspace", "CMY");
        break;
    default:
        m_colorspace = i18nc("colorspace", "Unknown");
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

    /* get profile header version */
    cmsFloat64Number profile_version;
    profile_version = cmsGetProfileVersion(m_lcmsProfile);
    m_version = QString::number(profile_version, 'f', 2);

    /* allocate temporary buffer */
    wchar_t *text = new wchar_t[1024];

    /* get description */
    ret = cmsGetProfileInfo(m_lcmsProfile, cmsInfoDescription, "en", "US", text, 1024);
    if (ret) {
        m_description = QString::fromWCharArray(text).simplified();
        if (m_description.isEmpty()) {
            m_description = i18n("Missing description");
        }
    }

    /* get copyright */
    ret = cmsGetProfileInfo(m_lcmsProfile, cmsInfoCopyright, "en", "US", text, 1024);
    if (ret) {
        m_copyright = QString::fromWCharArray(text).simplified();
    }

    /* get description */
    ret = cmsGetProfileInfo(m_lcmsProfile, cmsInfoManufacturer, "en", "US", text, 1024);
    if (ret) {
        m_manufacturer = QString::fromWCharArray(text).simplified();

    }

    /* get description */
    ret = cmsGetProfileInfo(m_lcmsProfile, cmsInfoModel, "en", "US", text, 1024);
    if (ret) {
        m_model = QString::fromWCharArray(text).simplified();
    }

    delete [] text;

//    /* success */
//    ret = TRUE;

    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(reinterpret_cast<const char *>(data), length);
    m_checksum = hash.result().toHex();
    qDebug() << "checksum" << m_checksum;

//    /* generate and set checksum */
//    checksum = g_compute_checksum_for_data (G_CHECKSUM_MD5, (const guchar *) data, length);
//    gcm_profile_set_checksum (profile, checksum);
//out:
//    g_free (text);
//    g_free (checksum);
//    return ret;
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
        return i18nc("profile kind", "Input device");
    case KindDisplayDevice:
        return i18nc("profile kind", "Display device");
    case KindOutputDevice:
        return i18nc("profile kind", "Output device");
    case KindDeviceLink:
        return i18nc("profile kind", "Devicelink");
    case KindColorspaceConversion:
        return i18nc("profile kind", "Colorspace conversion");
    case KindAbstract:
        return i18nc("profile kind", "Abstract");
    case KindNamedColor:
        return i18nc("profile kind", "Named color");
    default:
        return i18nc("profile kind", "Unknown");
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

QString Profile::checksum() const
{
    return m_checksum;
}

uint Profile::temperature() const
{
    return m_temperature;
}

QMap<QString, QColor> Profile::getNamedColors()
{
    QMap<QString, QColor> array;
    if (m_lcmsProfile == nullptr) {
        return array;
    }

    cmsCIELab lab;
    cmsCIEXYZ xyz;
    cmsHPROFILE profile_lab = nullptr;
    cmsHPROFILE profile_xyz = nullptr;
    cmsHTRANSFORM xform = nullptr;
    cmsNAMEDCOLORLIST *nc2 = nullptr;
    cmsUInt16Number pcs[3];
    cmsUInt32Number count;
    bool ret;
    char name[cmsMAX_PATH];
    char prefix[33];
    char suffix[33];

    /* setup a dummy transform so we can get all the named colors */
    profile_lab = cmsCreateLab2Profile(nullptr);
    profile_xyz = cmsCreateXYZProfile();
    xform = cmsCreateTransform(profile_lab, TYPE_Lab_DBL,
                               profile_xyz, TYPE_XYZ_DBL,
                               INTENT_ABSOLUTE_COLORIMETRIC, 0);
    if (xform == nullptr) {
        qWarning() << "no transform";
        goto out;
    }

    /* retrieve named color list from transform */
    nc2 = static_cast<cmsNAMEDCOLORLIST*>(cmsReadTag(m_lcmsProfile, cmsSigNamedColor2Tag));
    if (nc2 == nullptr) {
        qWarning() << "no named color list";
        goto out;
    }

    /* get the number of NCs */
    count = cmsNamedColorCount(nc2);
    if (count == 0) {
        qWarning() << "no named colors";
        goto out;
    }

    for (uint i = 0; i < count; ++i) {

        /* parse title */
        QString string;
        ret = cmsNamedColorInfo(nc2, i,
                                name,
                                prefix,
                                suffix,
                                (cmsUInt16Number *)&pcs,
                                nullptr);
        if (!ret) {
            qWarning() << "failed to get NC #" << i;
            goto out;
        }

//        if (prefix[0] != '\0') {
//            string.append(prefix);
        // Add a space if we got the above prefix
//        }
        string.append(name);
        if (suffix[0] != '\0') {
            string.append(suffix);
        }

        /* get color */
        cmsLabEncoded2Float((cmsCIELab *) &lab, pcs);
        cmsDoTransform(xform, &lab, &xyz, 1);

        QColor color;
        color = convertXYZ(&xyz);
        if (!color.isValid()) {
            continue;
        }

        // Store the color
        array[string] = color;
    }

out:
    if (profile_lab != nullptr)
        cmsCloseProfile(profile_lab);
    if (profile_xyz != nullptr)
        cmsCloseProfile(profile_xyz);
    if (xform != nullptr)
        cmsDeleteTransform(xform);
    return array;
}

QString Profile::profileWithSource(const QString &dataSource, const QString &profilename, const QDateTime &created)
{
    if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_EDID)) {
        return i18n("Default: %1", profilename);
    } else if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_STANDARD)) {
        return i18n("Colorspace: %1", profilename);
    } else if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_TEST)) {
        return i18n("Test profile: %1", profilename);
    } else if (dataSource == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE_CALIB)) {
        return i18n("%1 (%2)", profilename, QLocale().toString(created, QLocale::LongFormat));
    }
    return profilename;
}
