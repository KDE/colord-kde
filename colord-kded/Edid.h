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

#ifndef EDID_H
#define EDID_H

#include <QtGlobal>
#include <QQuaternion>

class Edid
{
public:
    Edid();
    bool parse(const quint8 *data, size_t length);

private:
    int edidGetBit(int in, int bit) const;
    int edidGetBits(int in, int begin, int end) const;
    double edidDecodeFraction(int high, int low) const;
    QString edidParseString(const quint8 *data) const;

    QString                        m_monitor_name;
    QString                        m_vendor_name;
    QString                        m_serial_number;
    QString                        m_eisa_id;
    char                           *m_checksum;
    char                           *m_pnp_id;
    uint                            m_width;
    uint                            m_height;
    float                           m_gamma;
    QQuaternion m_red;
    QQuaternion m_green;
    QQuaternion m_blue;
    QQuaternion m_white;
//    GnomePnpIds                     *pnp_ids;
};

#endif // EDID_H
