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

#include "ProfileDescription.h"
#include "ui_ProfileDescription.h"

#include "Profile.h"

#include <KDebug>


ProfileDescription::ProfileDescription(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProfileDescription)
{
    ui->setupUi(this);
}

ProfileDescription::~ProfileDescription()
{
    delete ui;
}

void ProfileDescription::setFilename(const QString &filename)
{
    Profile profile(filename);
    if (profile.loaded()) {
        ui->versionL->setText(profile.version());
        kDebug() << profile.datetime();
        kDebug() << profile.description();
    }
    kDebug() << profile.filename();
}

#include "ProfileDescription.moc"
