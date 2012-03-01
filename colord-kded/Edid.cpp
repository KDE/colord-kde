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

#include "Edid.h"

#include <math.h>

#include <KDebug>

#define GCM_EDID_OFFSET_PNPID                           0x08
#define GCM_EDID_OFFSET_SERIAL                          0x0c
#define GCM_EDID_OFFSET_SIZE                            0x15
#define GCM_EDID_OFFSET_GAMMA                           0x17
#define GCM_EDID_OFFSET_DATA_BLOCKS                     0x36
#define GCM_EDID_OFFSET_LAST_BLOCK                      0x6c
#define GCM_EDID_OFFSET_EXTENSION_BLOCK_COUNT           0x7e

#define GCM_DESCRIPTOR_DISPLAY_PRODUCT_NAME             0xfc
#define GCM_DESCRIPTOR_DISPLAY_PRODUCT_SERIAL_NUMBER    0xff
#define GCM_DESCRIPTOR_COLOR_MANAGEMENT_DATA            0xf9
#define GCM_DESCRIPTOR_ALPHANUMERIC_DATA_STRING         0xfe
#define GCM_DESCRIPTOR_COLOR_POINT                      0xfb

Edid::Edid() :
    m_valid(false)
{
}

Edid::Edid(const quint8 *data, size_t length)
{
    parse(data, length);
}

bool Edid::isValid() const
{
    return m_valid;
}

QString Edid::deviceId(const QString &fallbackName) const
{
    QString id = QLatin1String("xrandr");
    if (m_valid) {
        if (!vendor().isEmpty()) {
            id.append(QLatin1Char('-') + vendor());
        }
        if (!name().isEmpty()) {
            id.append(QLatin1Char('-') + name());
        }
        if (!serial().isEmpty()) {
            id.append(QLatin1Char('-') + serial());
        }
    }

    // if no info was added check if the fallbacName is provided
    if (id == QLatin1String("xrandr")) {
        if (!fallbackName.isEmpty()) {
            id.append(QLatin1Char('-') + fallbackName);
        } else {
            // all info we have are empty strings
            id.append(QLatin1String("-unknown"));
        }
    }

    return id;
}

QString Edid::name() const
{
    if (m_valid) {
        return m_monitorName;
    }
    return QString();
}

QString Edid::vendor() const
{
    if (m_valid) {
        return m_vendorName;
    }
    return QString();
}

QString Edid::serial() const
{
    if (m_valid) {
        return m_serialNumber;
    }
    return QString();
}

QString Edid::eisaId() const
{
    if (m_valid) {
        return m_eisaId;
    }
    return QString();
}

uint Edid::width() const
{
    return m_width;
}

uint Edid::height() const
{
    return m_height;
}

float Edid::gamma() const
{
    return m_gamma;
}

QQuaternion Edid::red() const
{
    return m_red;
}

QQuaternion Edid::green() const
{
    return m_green;
}

QQuaternion Edid::blue() const
{
    return m_blue;
}

QQuaternion Edid::white() const
{
    return m_white;
}

bool Edid::parse(const quint8 *data, size_t length)
{
    uint i;
//    GcmEdidPrivate *priv = edid->priv;
    quint32 serial;
//    char *tmp;

    /* check header */
    if (length < 128) {
        kWarning() << "EDID length is too small";
//        g_set_error_literal (error,
//                             GCM_EDID_ERROR,
//                             GCM_EDID_ERROR_FAILED_TO_PARSE,
//                             "EDID length is too small");
        m_valid = false;
        return m_valid;
    }
    if (data[0] != 0x00 || data[1] != 0xff) {
        kWarning() << "Failed to parse EDID header";
//        g_set_error_literal (error,
//                             GCM_EDID_ERROR,
//                             GCM_EDID_ERROR_FAILED_TO_PARSE,
//                             "Failed to parse EDID header");
        m_valid = false;
        return m_valid;
    }

    /* free old data */
//    gcm_edid_reset (edid);

    /* decode the PNP ID from three 5 bit words packed into 2 bytes
             * /--08--\/--09--\
             * 7654321076543210
             * |\---/\---/\---/
             * R  C1   C2   C3 */
//    priv->pnp_id[0] = 'A' + ((data[GCM_EDID_OFFSET_PNPID + 0] & 0x7c) / 4) - 1;
//    priv->pnp_id[1] = 'A' + ((data[GCM_EDID_OFFSET_PNPID + 0] & 0x3) * 8) + ((data[GCM_EDID_OFFSET_PNPID+1] & 0xe0) / 32) - 1;
//    priv->pnp_id[2] = 'A' + (data[GCM_EDID_OFFSET_PNPID + 1] & 0x1f) - 1;

    /* maybe there isn't a ASCII serial number descriptor, so use this instead */
    serial = static_cast<quint32>(data[GCM_EDID_OFFSET_SERIAL + 0]);
    serial += static_cast<quint32>(data[GCM_EDID_OFFSET_SERIAL + 1] * 0x100);
    serial += static_cast<quint32>(data[GCM_EDID_OFFSET_SERIAL + 2] * 0x10000);
    serial += static_cast<quint32>(data[GCM_EDID_OFFSET_SERIAL + 3] * 0x1000000);
//    if (serial > 0)
//        priv->serial_number = g_strdup_printf ("%" G_GUINT32_FORMAT, serial);

    /* get the size */
    m_width = data[GCM_EDID_OFFSET_SIZE + 0];
    m_height = data[GCM_EDID_OFFSET_SIZE + 1];
    kDebug() << "width" << m_width << "height" << m_height;

    /* we don't care about aspect */
    if (m_width == 0 || m_height == 0) {
        m_width = 0;
        m_height = 0;
    }

    /* get gamma */
    if (data[GCM_EDID_OFFSET_GAMMA] == 0xff) {
        m_gamma = 1.0f;
    } else {
        m_gamma = (static_cast<float>(data[GCM_EDID_OFFSET_GAMMA] / 100) + 1);
    }
    kDebug() << "Gama" << m_gamma;

    /* get color red */
    m_red.setX(edidDecodeFraction(data[0x1b], edidGetBits(data[0x19], 6, 7)));
    m_red.setY(edidDecodeFraction(data[0x1c], edidGetBits(data[0x19], 5, 4)));
    kDebug() << "red" << m_red;

    /* get color green */
    m_green.setX(edidDecodeFraction(data[0x1d], edidGetBits(data[0x19], 2, 3)));
    m_green.setY(edidDecodeFraction(data[0x1e], edidGetBits(data[0x19], 0, 1)));
    kDebug() << "m_green" << m_green;

    /* get color blue */
    m_blue.setX(edidDecodeFraction(data[0x1f], edidGetBits(data[0x1a], 6, 7)));
    m_blue.setY(edidDecodeFraction(data[0x20], edidGetBits(data[0x1a], 4, 5)));
    kDebug() << "m_blue" << m_blue;

    /* get color white */
    m_white.setX(edidDecodeFraction(data[0x21], edidGetBits(data[0x1a], 2, 3)));
    m_white.setY(edidDecodeFraction(data[0x22], edidGetBits(data[0x1a], 0, 1)));
    kDebug() << "m_white" << m_white;

    /* parse EDID data */
    for (i = GCM_EDID_OFFSET_DATA_BLOCKS;
         i <= GCM_EDID_OFFSET_LAST_BLOCK;
         i += 18) {
        /* ignore pixel clock data */
        if (data[i] != 0) {
            continue;
        }
        if (data[i+2] != 0) {
            continue;
        }

        /* any useful blocks? */
        if (data[i+3] == GCM_DESCRIPTOR_DISPLAY_PRODUCT_NAME) {
            m_monitorName = edidParseString(&data[i+5]);
            kDebug() << "m_monitorName" << m_monitorName;
//            if (tmp != NULL) {
//                g_free (priv->monitor_name);
//                priv->monitor_name = tmp;
//            }
        } else if (data[i+3] == GCM_DESCRIPTOR_DISPLAY_PRODUCT_SERIAL_NUMBER) {
            m_serialNumber = edidParseString(&data[i+5]);
            kDebug() << "m_serialNumber" << m_serialNumber;
//            if (tmp != NULL) {
//                g_free (priv->serial_number);
//                priv->serial_number = tmp;
//            }
        } else if (data[i+3] == GCM_DESCRIPTOR_COLOR_MANAGEMENT_DATA) {
            kWarning() << "failing to parse color management data";
        } else if (data[i+3] == GCM_DESCRIPTOR_ALPHANUMERIC_DATA_STRING) {
            m_eisaId = edidParseString(&data[i+5]);
            kDebug() << "m_eisaId" << m_eisaId;
//            if (tmp != NULL) {
//                g_free (priv->eisa_id);
//                priv->eisa_id = tmp;
//            }
        } else if (data[i+3] == GCM_DESCRIPTOR_COLOR_POINT) {
            if (data[i+3+9] != 0xff) {
                /* extended EDID block(1) which contains
                                     * a better gamma value */
                m_gamma = ((float) data[i+3+9] / 100) + 1;
                kDebug() << "m_gamma2" << m_gamma;
            }
            if (data[i+3+14] != 0xff) {
                /* extended EDID block(2) which contains
                                     * a better gamma value */
                m_gamma = ((float) data[i+3+9] / 100) + 1;
                kDebug() << "m_gamma3" << m_gamma;
            }
        }
    }

    /* calculate checksum */
//    m_checksum = g_compute_checksum_for_data (G_CHECKSUM_MD5, data, 0x6c);
//out:
    m_valid = true;
    return m_valid;
}

int Edid::edidGetBit(int in, int bit) const
{
        return (in & (1 << bit)) >> bit;
}

int Edid::edidGetBits(int in, int begin, int end) const
{
        int mask = (1 << (end - begin + 1)) - 1;

        return (in >> begin) & mask;
}

double Edid::edidDecodeFraction(int high, int low) const
{
        double result = 0.0;
        int i;

        high = (high << 2) | low;
        for (i = 0; i < 10; ++i) {
            result += edidGetBit(high, i) * pow(2, i - 10);
        }
        return result;
}

QString Edid::edidParseString(const quint8 *data) const
{
        QString text;
//        uint i;
//        uint replaced = 0;

        /* this is always 12 bytes, but we can't guarantee it's null
         * terminated or not junk. */
        text = QString::fromLocal8Bit((const char*) data, 12);
        kDebug() << "text" << text;
//        text = g_strndup ((const gchar *) data, 12);

        /* remove insane newline chars */
        text = text.simplified();
//        g_strdelimit (text, "\n\r", '\0');

        /* remove spaces */
//        g_strchomp (text);

        /* nothing left? */
//        if (text[0] == '\0') {
//                g_free (text);
//                text = NULL;
//                goto out;
//        }

        /* ensure string is printable */
//        for (i = 0; text[i] != '\0'; i++) {
//                if (!g_ascii_isprint (text[i])) {
//                        text[i] = '-';
//                        replaced++;
//                }
//        }

        /* if the string is junk, ignore the string */
//        if (replaced > 4) {
//                g_free (text);
//                text = NULL;
//                goto out;
//        }
        kDebug() << "text" << text;
//out:
        return text;
}
