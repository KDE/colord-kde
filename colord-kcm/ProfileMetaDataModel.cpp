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

#include "ProfileMetaDataModel.h"

#include <QDebug>
#include <klocalizedstring.h>
/* defined in metadata-spec.txt */
#define CD_PROFILE_METADATA_STANDARD_SPACE QStringLiteral("STANDARD_space")
#define CD_PROFILE_METADATA_EDID_MD5 QStringLiteral("EDID_md5")
#define CD_PROFILE_METADATA_EDID_MODEL QStringLiteral("EDID_model")
#define CD_PROFILE_METADATA_EDID_SERIAL QStringLiteral("EDID_serial")
#define CD_PROFILE_METADATA_EDID_MNFT QStringLiteral("EDID_mnft")
#define CD_PROFILE_METADATA_EDID_VENDOR QStringLiteral("EDID_manufacturer")
#define CD_PROFILE_METADATA_FILE_CHECKSUM QStringLiteral("FILE_checksum")
#define CD_PROFILE_METADATA_CMF_PRODUCT QStringLiteral("CMF_product")
#define CD_PROFILE_METADATA_CMF_BINARY QStringLiteral("CMF_binary")
#define CD_PROFILE_METADATA_CMF_VERSION QStringLiteral("CMF_version")
#define CD_PROFILE_METADATA_DATA_SOURCE QStringLiteral("DATA_source")
#define CD_PROFILE_METADATA_MAPPING_FORMAT QStringLiteral("MAPPING_format")
#define CD_PROFILE_METADATA_MAPPING_QUALIFIER QStringLiteral("MAPPING_qualifier")

ProfileMetaDataModel::ProfileMetaDataModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void ProfileMetaDataModel::setMetadata(const CdStringMap &metadata)
{
    beginResetModel();
    m_data = metadata;
    m_keys = m_data.keys();
    endResetModel();
    qDebug() << "set metadata" << m_data;
}

int ProfileMetaDataModel::rowCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return m_keys.count();
}
QVariant ProfileMetaDataModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_keys.count()) {
        return QVariant();
    }

    switch (role) {
    case TitleRole:
        return metadataLabel(m_keys[index.row()]);
    case ValueRole:
        return m_data[m_keys[index.row()]];
    }
    return QVariant();
}
QHash<int, QByteArray> ProfileMetaDataModel::roleNames() const
{
    return {{TitleRole, "title"}, {ValueRole, "textValue"}};
}

QString ProfileMetaDataModel::metadataLabel(const QString &key) const
{
    if (key == CD_PROFILE_METADATA_STANDARD_SPACE) {
        return i18n("Standard space");
    }
    if (key == CD_PROFILE_METADATA_EDID_MD5) {
        return i18n("Display checksum");
    }
    if (key == CD_PROFILE_METADATA_EDID_MODEL) {
        return i18n("Display model");
    }
    if (key == CD_PROFILE_METADATA_EDID_SERIAL) {
        return i18n("Display serial number");
    }
    if (key == CD_PROFILE_METADATA_EDID_MNFT) {
        return i18n("Display PNPID");
    }
    if (key == CD_PROFILE_METADATA_EDID_VENDOR) {
        return i18n("Display vendor");
    }
    if (key == CD_PROFILE_METADATA_FILE_CHECKSUM) {
        return i18n("File checksum");
    }
    if (key == CD_PROFILE_METADATA_CMF_PRODUCT) {
        return i18n("Framework product");
    }
    if (key == CD_PROFILE_METADATA_CMF_BINARY) {
        return i18n("Framework program");
    }
    if (key == CD_PROFILE_METADATA_CMF_VERSION) {
        return i18n("Framework version");
    }
    if (key == CD_PROFILE_METADATA_DATA_SOURCE) {
        return i18n("Data source type");
    }
    if (key == CD_PROFILE_METADATA_MAPPING_FORMAT) {
        return i18n("Mapping format");
    }
    if (key == CD_PROFILE_METADATA_MAPPING_QUALIFIER) {
        return i18n("Mapping qualifier");
    }
    return key;
}

#include "moc_ProfileMetaDataModel.cpp"
