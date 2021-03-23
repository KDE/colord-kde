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

#include "ProfileUtils.h"

#include "Edid.h"
#include "DmiUtils.h"

#include <QFileInfo>
#include <QCryptographicHash>

#include <QLoggingCategory>

#include <version.h>

#define PACKAGE_NAME PROJECT_NAME
#define PACKAGE_VERSION COLORD_KDE_VERSION_STRING

/* defined in metadata-spec.txt */
#define CD_PROFILE_METADATA_STANDARD_SPACE	"STANDARD_space"
#define CD_PROFILE_METADATA_EDID_MD5		"EDID_md5"
#define CD_PROFILE_METADATA_EDID_MODEL		"EDID_model"
#define CD_PROFILE_METADATA_EDID_SERIAL		"EDID_serial"
#define CD_PROFILE_METADATA_EDID_MNFT		"EDID_mnft"
#define CD_PROFILE_METADATA_EDID_VENDOR		"EDID_manufacturer"
#define CD_PROFILE_METADATA_FILE_CHECKSUM	"FILE_checksum"
#define CD_PROFILE_METADATA_CMF_PRODUCT		"CMF_product"
#define CD_PROFILE_METADATA_CMF_BINARY		"CMF_binary"
#define CD_PROFILE_METADATA_CMF_VERSION		"CMF_version"
#define CD_PROFILE_METADATA_DATA_SOURCE		"DATA_source"
#define CD_PROFILE_METADATA_DATA_SOURCE_EDID	"edid"
#define CD_PROFILE_METADATA_DATA_SOURCE_CALIB	"calib"
#define CD_PROFILE_METADATA_MAPPING_FORMAT	"MAPPING_format"
#define CD_PROFILE_METADATA_MAPPING_QUALIFIER	"MAPPING_qualifier"

Q_DECLARE_LOGGING_CATEGORY(COLORD)

QString ProfileUtils::profileHash(QFile &profile)
{
    QString checksum;
    cmsHPROFILE lcms_profile = NULL;

    /* get the internal profile id, if it exists */
    lcms_profile = cmsOpenProfileFromFile(profile.fileName().toUtf8(), "r");
    if (lcms_profile == NULL) {
        // Compute the hash from the whole file..
        return QCryptographicHash::hash(profile.readAll(), QCryptographicHash::Md5).toHex();
    } else {
        checksum = getPrecookedMd5(lcms_profile);
        if (lcms_profile != NULL) {
            cmsCloseProfile (lcms_profile);
        }

        if (checksum.isNull()) {
            // Compute the hash from the whole file..
            return QCryptographicHash::hash(profile.readAll(), QCryptographicHash::Md5).toHex();
        } else {
            return checksum;
        }
    }
}

QString ProfileUtils::getPrecookedMd5(cmsHPROFILE lcms_profile)
{
    cmsUInt8Number profile_id[16];
    bool md5_precooked = false;
    QByteArray md5;

    /* check to see if we have a pre-cooked MD5 */
    cmsGetHeaderProfileID(lcms_profile, profile_id);
    for (int i = 0; i < 16; ++i) {
        if (profile_id[i] != 0) {
            md5_precooked = true;
            break;
        }
    }
    if (!md5_precooked) {
        return QString();
    }

    /* convert to a hex string */
    for (int i = 0; i < 16; ++i) {
        md5.append(profile_id[i]);
    }

    return md5.toHex();
}

bool ProfileUtils::createIccProfile(bool isLaptop, const Edid &edid, const QString &filename)
{
    cmsCIExyYTRIPLE chroma;
    cmsCIExyY white_point;
    cmsHPROFILE lcms_profile = NULL;
    cmsToneCurve *transfer_curve[3] = { NULL, NULL, NULL };
    bool ret = false;
    cmsHANDLE dict = NULL;

    /* ensure the per-user directory exists */
    // Create dir path if not available
    // check if the file doesn't already exist
    QFileInfo fileInfo(filename);
    if (fileInfo.exists()) {
        qCWarning(COLORD) << "EDID ICC Profile already exists" << filename;
        if (*transfer_curve != NULL)
            cmsFreeToneCurve(*transfer_curve);
        return false;
    }

    // copy color data from our structures
    // Red
    chroma.Red.x = edid.red().x();
    chroma.Red.y = edid.red().y();
    // Green
    chroma.Green.x = edid.green().x();
    chroma.Green.y = edid.green().y();
    // Blue
    chroma.Blue.x = edid.blue().x();
    chroma.Blue.y = edid.blue().y();
    // White
    white_point.x = edid.white().x();
    white_point.y = edid.white().y();
    white_point.Y = 1.0;

    // estimate the transfer function for the gamma
    transfer_curve[0] = transfer_curve[1] = transfer_curve[2] = cmsBuildGamma(NULL, edid.gamma());

    // create our generated profile
    lcms_profile = cmsCreateRGBProfile(&white_point, &chroma, transfer_curve);
    if (lcms_profile == NULL) {
        qCWarning(COLORD) << "Failed to create ICC profile on cmsCreateRGBProfile";
        if (*transfer_curve != NULL)
            cmsFreeToneCurve(*transfer_curve);
        return false;
    }

    cmsSetColorSpace(lcms_profile, cmsSigRgbData);
    cmsSetPCS(lcms_profile, cmsSigXYZData);
    cmsSetHeaderRenderingIntent(lcms_profile, INTENT_RELATIVE_COLORIMETRIC);
    cmsSetDeviceClass(lcms_profile, cmsSigDisplayClass);

    // copyright
    ret = cmsWriteTagTextAscii(lcms_profile,
                               cmsSigCopyrightTag,
                               "No copyright");
    if (!ret) {
        qCWarning(COLORD) << "Failed to write copyright";
        if (*transfer_curve != NULL)
            cmsFreeToneCurve(*transfer_curve);
        return false;
    }

    // set model
    QString model;
    if (isLaptop) {
        model = DmiUtils::deviceModel();
    } else {
        model = edid.name();
    }

    if (model.isEmpty()) {
        model = QStringLiteral("Unknown monitor");
    }
    ret = cmsWriteTagTextAscii(lcms_profile,
                               cmsSigDeviceModelDescTag,
                               model);
    if (!ret) {
        qCWarning(COLORD) << "Failed to write model";
        if (*transfer_curve != NULL) {
            cmsFreeToneCurve(*transfer_curve);
        }
        return false;
    }

    // write title
    ret = cmsWriteTagTextAscii(lcms_profile,
                               cmsSigProfileDescriptionTag,
                               model);
    if (!ret) {
        qCWarning(COLORD) << "Failed to write description";
        if (*transfer_curve != NULL)
            cmsFreeToneCurve(*transfer_curve);
        return false;
    }

    // get manufacturer
    QString vendor;
    if (isLaptop) {
        vendor = DmiUtils::deviceVendor();
    } else {
        vendor = edid.vendor();
    }

    if (vendor.isEmpty()) {
        vendor = QStringLiteral("Unknown vendor");
    }
    ret = cmsWriteTagTextAscii(lcms_profile,
                               cmsSigDeviceMfgDescTag,
                               vendor);
    if (!ret) {
        qCWarning(COLORD) << "Failed to write manufacturer";
        if (*transfer_curve != NULL)
            cmsFreeToneCurve(*transfer_curve);
        return false;
    }

    // just create a new dict
    dict = cmsDictAlloc(NULL);

    // set the framework creator metadata
    cmsDictAddEntryAscii(dict,
                         CD_PROFILE_METADATA_CMF_PRODUCT,
                         PACKAGE_NAME);
    cmsDictAddEntryAscii(dict,
                         CD_PROFILE_METADATA_CMF_BINARY,
                         PACKAGE_NAME);
    cmsDictAddEntryAscii(dict,
                         CD_PROFILE_METADATA_CMF_VERSION,
                         PACKAGE_VERSION);

    /* set the data source so we don't ever prompt the user to
         * recalibrate (as the EDID data won't have changed) */
    cmsDictAddEntryAscii(dict,
                         CD_PROFILE_METADATA_DATA_SOURCE,
                         CD_PROFILE_METADATA_DATA_SOURCE_EDID);

    // set 'ICC meta Tag for Monitor Profiles' data
    cmsDictAddEntryAscii(dict, "EDID_md5", edid.hash());

    if (!model.isEmpty())
        cmsDictAddEntryAscii(dict, "EDID_model", model);

    if (!edid.serial().isEmpty()) {
        cmsDictAddEntryAscii(dict, "EDID_serial", edid.serial());
    }

    if (!edid.pnpId().isEmpty()) {
        cmsDictAddEntryAscii(dict, "EDID_mnft", edid.pnpId());
    }

    if (!vendor.isEmpty()) {
        cmsDictAddEntryAscii(dict, "EDID_manufacturer", vendor);
    }

    /* write new tag */
    ret = cmsWriteTag(lcms_profile, cmsSigMetaTag, dict);
    if (!ret) {
        qCWarning(COLORD) << "Failed to write profile metadata";
        if (*transfer_curve != NULL)
            cmsFreeToneCurve(*transfer_curve);
        return false;
    }

    /* write profile id */
    ret = cmsMD5computeID(lcms_profile);
    if (!ret) {
        qCWarning(COLORD) << "Failed to write profile id";
        if (dict != NULL)
            cmsDictFree (dict);
        if (*transfer_curve != NULL)
            cmsFreeToneCurve(*transfer_curve);
        return false;
    }

    /* save, TODO: get error */
    ret = cmsSaveProfileToFile(lcms_profile, filename.toUtf8());

    if (dict != NULL) {
        cmsDictFree (dict);
    }
    if (*transfer_curve != NULL) {
        cmsFreeToneCurve (*transfer_curve);
    }

    return ret;
}

cmsBool ProfileUtils::cmsWriteTagTextAscii(cmsHPROFILE lcms_profile,
                                           cmsTagSignature sig,
                                           const QString &text)
{
    cmsBool ret;
    cmsMLU *mlu = cmsMLUalloc(0, 1);
    cmsMLUsetASCII(mlu, "EN", "us", text.toLatin1().constData());
    ret = cmsWriteTag(lcms_profile, sig, mlu);
    cmsMLUfree(mlu);
    return ret;
}

cmsBool ProfileUtils::cmsDictAddEntryAscii(cmsHANDLE dict,
                                           const QString &key,
                                           const QString &value)
{
    qCDebug(COLORD) << key << value;
    cmsBool ret;

    wchar_t *mb_key = new wchar_t[key.length() + 1];
    if (key.toWCharArray(mb_key) != key.length()) {
        delete [] mb_key;
        return false;
    }
    mb_key[key.length()] = 0;

    wchar_t *mb_value = new wchar_t[value.length() + 1];
    if (value.toWCharArray(mb_value) != value.length()) {
        delete [] mb_key;
        delete [] mb_value;
        return false;
    }
    mb_value[value.length()] = 0;

    ret = cmsDictAddEntry(dict, mb_key, mb_value, NULL, NULL);
    delete [] mb_key;
    delete [] mb_value;
    return ret;
}
