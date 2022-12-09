/***************************************************************************
 *   Copyright (C) 2012-2016 by Daniel Nicoletti <dantti12@gmail.com>      *
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

#include "ProfileNamedColorsModel.h"
#include <QColor>
ProfileNamedColorsModel::ProfileNamedColorsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void ProfileNamedColorsModel::setNamedColors(const QMap<QString, QColor> &namedColors)
{
    beginResetModel();
    m_data = namedColors;
    m_keys = m_data.keys();
    endResetModel();
}

int ProfileNamedColorsModel::rowCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return m_keys.count();
}
QVariant ProfileNamedColorsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_keys.count()) {
        return QVariant();
    }
    switch (role) {
    case NameRole:
        return m_keys[index.row()];
    case ColorRole:
        return m_data[m_keys[index.row()]];
    case IsDarkColorRole:
        auto color = m_data[m_keys[index.row()]];
        // 0.2126*r + 0.7151*g + 0.0721*b
        int y = 0.2126 * color.red() + 0.7151 * color.green() + 0.0721 * color.blue();
        return y < 140;
    }
    return QVariant();
}
QHash<int, QByteArray> ProfileNamedColorsModel::roleNames() const
{
    return {{NameRole, "name"}, {ColorRole, "colorValue"}, {IsDarkColorRole, "isDarkColor"}};
}
