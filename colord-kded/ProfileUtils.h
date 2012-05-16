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

#ifndef PROFILEUTILS_H
#define PROFILEUTILS_H

#include <lcms2.h>

#include <QString>
#include <QFile>

class Edid;
class ProfileUtils
{
public:
    static QString profileHash(QFile &profile);
    static bool createIccProfile(bool isLaptop, const Edid &edid, const QString &filename);

private:
    static QString getPrecookedMd5(cmsHPROFILE lcms_profile);
    static cmsBool cmsWriteTagTextAscii(cmsHPROFILE lcms_profile,
                                        cmsTagSignature sig,
                                        const QString &text);
    static cmsBool cmsDictAddEntryAscii(cmsHANDLE dict,
                                        const QString &tkey,
                                        const QString &tvalue);
};

#endif // PROFILEUTILS_H
