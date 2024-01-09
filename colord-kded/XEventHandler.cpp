/***************************************************************************
 *   Copyright (C) 2012-2016 by Daniel Nicoletti <dantti12@gmail.com>      *
 *   Copyright (C) 2015 Lukáš Tinkl <ltinkl@redhat.com>                    *
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

#include <QCoreApplication>

#include <xcb/randr.h>

XEventHandler::XEventHandler(int randrBase)
    : m_randrBase(randrBase)
{
    qApp->installNativeEventFilter(this);
}

bool XEventHandler::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);
    if (eventType != "xcb_generic_event_t") {
        // only interested in XCB  events
        return false;
    }
    auto e = static_cast<xcb_generic_event_t *>(message);
    auto xEventType = e->response_type & ~0x80;

    if (xEventType == m_randrBase + XCB_RANDR_SCREEN_CHANGE_NOTIFY) {
        emit outputChanged();
    }

    return false;
}

#include "moc_XEventHandler.cpp"
