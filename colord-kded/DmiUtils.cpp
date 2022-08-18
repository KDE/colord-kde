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

#include "DmiUtils.h"

#include <QFile>
#include <QStringList>

QString DmiUtils::deviceModel()
{
    QString ret;
    const QStringList sysfsNames = {QStringLiteral("/sys/class/dmi/id/product_name"), QStringLiteral("/sys/class/dmi/id/board_name")};

    for (const QString &filename : sysfsNames) {
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly)) {
            QString tmp = file.readAll();
            tmp = tmp.simplified();
            if (!tmp.isEmpty()) {
                ret = tmp;
                break;
            }
        }
    }
    return ret;
}

QString DmiUtils::deviceVendor()
{
    QString ret;
    const QStringList sysfsVendors = {QStringLiteral("/sys/class/dmi/id/sys_vendor"),
                                      QStringLiteral("/sys/class/dmi/id/chassis_vendor"),
                                      QStringLiteral("/sys/class/dmi/id/board_vendor")};

    for (const QString &filename : sysfsVendors) {
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly)) {
            QString tmp = file.readAll().simplified();
            tmp = tmp.simplified();
            if (!tmp.isEmpty()) {
                ret = tmp;
                break;
            }
        }
    }
    return ret;
}
