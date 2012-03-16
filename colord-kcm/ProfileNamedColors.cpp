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

#include "ProfileNamedColors.h"
#include "ui_ProfileNamedColors.h"

#include <QColor>

#include <KDebug>

ProfileNamedColors::ProfileNamedColors(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProfileNamedColors)
{
    ui->setupUi(this);

    m_model = new QStandardItemModel(this);
    ui->treeView->setModel(m_model);
}

ProfileNamedColors::~ProfileNamedColors()
{
    delete ui;
}

void ProfileNamedColors::setNamedColors(const QMap<QString, QQuaternion> &namedColors)
{
    QMap<QString, QQuaternion>::const_iterator i = namedColors.constBegin();
    while (i != namedColors.constEnd()) {
        kDebug() << i.key() << ": " << i.value();
//        QStandardItem *name = new QStandardItem(i.key());

//        QStandardItem *color = new QStandardItem(i.key());
        ++i;
    }
}
