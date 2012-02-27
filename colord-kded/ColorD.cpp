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

#include "ColorD.h"

#include <KLocale>
#include <KGenericFactory>
#include <KNotification>
#include <KIcon>

#include <QX11Info>

K_PLUGIN_FACTORY(ColorDFactory, registerPlugin<ColorD>();)
K_EXPORT_PLUGIN(ColorDFactory("colord"))

bool has_1_2 = false;
bool has_1_3 = false;

ColorD::ColorD(QObject *parent, const QVariantList &args) :
    KDEDModule(parent)
{
    // There's not much use for args in a KCM
    Q_UNUSED(args)

    m_dpy = QX11Info::display();
    
    // Check extension
    if(XRRQueryExtension(m_dpy, &m_eventBase, &m_errorBase) == False) {
        m_valid = false;
        return;
    }
    
    int major_version, minor_version;
    XRRQueryVersion(m_dpy, &major_version, &minor_version);
    
    m_version = i18n("X Resize and Rotate extension version %1.%2",
                     major_version,minor_version);
    
    // check if we have the new version of the XRandR extension
    has_1_2 = (major_version > 1 || (major_version == 1 && minor_version >= 2));
    has_1_3 = (major_version > 1 || (major_version == 1 && minor_version >= 3));
    
    if(has_1_3)
        kDebug() << "Using XRANDR extension 1.3 or greater.";
    else if(has_1_2)
        kDebug() << "Using XRANDR extension 1.2.";
    else kDebug() << "Using legacy XRANDR extension (1.1 or earlier).";
    
    kDebug() << "XRANDR error base: " << m_errorBase;
    m_numScreens = ScreenCount(m_dpy);
    m_currentScreenIndex = 0;
    
    // set the timestamp to 0
//     RandR::timestamp = 0;
    
    // This assumption is WRONG with Xinerama
    // Q_ASSERT(QApplication::desktop()->numScreens() == ScreenCount(QX11Info::display()));
    
    for (int i = 0; i < m_numScreens; i++) {
        kDebug() << i;
//        TODO ok no clue
//        XRRQueryOutputProperty(m_dpy, i, RR_PROPERTY_RANDR_EDID);
    }
}

ColorD::~ColorD()
{
}
