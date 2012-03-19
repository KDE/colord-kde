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

#ifndef PROFILE_DESCRIPTION_H
#define PROFILE_DESCRIPTION_H

#include <QWidget>
#include <QDBusObjectPath>

namespace Ui {
    class ProfileDescription;
}
class ProfileNamedColors;
class ProfileMetaData;
class ProfileDescription : public QWidget
{
    Q_OBJECT
public:
    explicit ProfileDescription(QWidget *parent = 0);
    ~ProfileDescription();

    int innerHeight() const;
    void setProfile(const QDBusObjectPath &objectPath);

private:
    void insertTab(int index, QWidget *widget, const QString &label);
    void removeTab(QWidget *widget);

    Ui::ProfileDescription *ui;
    ProfileNamedColors *m_namedColors;
    ProfileMetaData *m_metadata;
};

#endif // PROFILE_DESCRIPTION_H
