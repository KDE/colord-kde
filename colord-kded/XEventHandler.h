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

#ifndef XEVENTHANDLER_H
#define XEVENTHANDLER_H

#include <QAbstractNativeEventFilter>
#include <QObject>

class Q_DECL_EXPORT XEventHandler : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    XEventHandler(int randrBase);

signals:
    void outputChanged();

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long int*) override;

private:
    int m_randrBase;
};

#endif // XEVENTHANDLER_H
