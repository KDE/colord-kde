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

#include "XEventHandler.h"

#include <KDebug>

#include <X11/extensions/Xrandr.h>

XEventHandler::XEventHandler(int randr_base) :
    m_randr_notify(randr_base + RRNotify)
{
}

bool XEventHandler::x11Event(XEvent *event)
{
    if (event->xany.type == m_randr_notify) {
        XRRNotifyEvent *notifyEvent = reinterpret_cast<XRRNotifyEvent*>(event);
        if(notifyEvent->subtype == RRNotify_OutputChange) {
            emit outputChanged();
        }
    }
    return QWidget::x11Event(event);
}
