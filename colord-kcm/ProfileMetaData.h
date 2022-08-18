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

#ifndef PROFILEMETADATA_H
#define PROFILEMETADATA_H

#include <QStandardItemModel>
#include <QWidget>

#include "dbus-types.h"

namespace Ui
{
class ProfileMetaData;
}

class ProfileMetaData : public QWidget
{
    Q_OBJECT
public:
    explicit ProfileMetaData(QWidget *parent = nullptr);
    ~ProfileMetaData() override;

    void setMetadata(const CdStringMap &metadata);

private:
    QString metadataLabel(const QString &key);

    Ui::ProfileMetaData *const ui;
    QStandardItemModel *m_model = nullptr;
};

#endif // PROFILEMETADATA_H
