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

#include "ProfileMetaData.h"
#include "ui_ProfileMetaData.h"

#include <KDebug>

/* defined in metadata-spec.txt */
#define CD_PROFILE_METADATA_STANDARD_SPACE	"STANDARD_space"
#define CD_PROFILE_METADATA_EDID_MD5		"EDID_md5"
#define CD_PROFILE_METADATA_EDID_MODEL		"EDID_model"
#define CD_PROFILE_METADATA_EDID_SERIAL		"EDID_serial"
#define CD_PROFILE_METADATA_EDID_MNFT		"EDID_mnft"
#define CD_PROFILE_METADATA_EDID_VENDOR		"EDID_manufacturer"
#define CD_PROFILE_METADATA_FILE_CHECKSUM	"FILE_checksum"
#define CD_PROFILE_METADATA_CMF_PRODUCT		"CMF_product"
#define CD_PROFILE_METADATA_CMF_BINARY		"CMF_binary"
#define CD_PROFILE_METADATA_CMF_VERSION		"CMF_version"
#define CD_PROFILE_METADATA_DATA_SOURCE		"DATA_source"
#define CD_PROFILE_METADATA_MAPPING_FORMAT	"MAPPING_format"
#define CD_PROFILE_METADATA_MAPPING_QUALIFIER	"MAPPING_qualifier"

ProfileMetaData::ProfileMetaData(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProfileMetaData)
{
    ui->setupUi(this);

    m_model = new QStandardItemModel(this);
    m_model->setColumnCount(2);
    ui->treeView->setModel(m_model);
    ui->treeView->header()->setResizeMode(0, QHeaderView::ResizeToContents);
}

ProfileMetaData::~ProfileMetaData()
{
    delete ui;
}

void ProfileMetaData::setMetadata(const QMap<QString, QString> &metadata)
{
    m_model->removeRows(0, m_model->rowCount());

    QMap<QString, QString>::const_iterator i = metadata.constBegin();
    while (i != metadata.constEnd()) {
        kDebug() << i.key() << ": " << i.value();
        QList<QStandardItem *> row;
        QStandardItem *name = new QStandardItem(metadataLabel(i.key()));
        row << name;
        QStandardItem *value = new QStandardItem(i.value());
        row << value;

        m_model->appendRow(row);
        ++i;
    }
}

QString ProfileMetaData::metadataLabel(const QString &key)
{
    if (key == QLatin1String(CD_PROFILE_METADATA_STANDARD_SPACE)) {
        return i18n("Standard space");
    }
    if (key == QLatin1String(CD_PROFILE_METADATA_EDID_MD5)) {
        return i18n("Display checksum");
    }
    if (key == QLatin1String(CD_PROFILE_METADATA_EDID_MODEL)) {
        return i18n("Display model");
    }
    if (key == QLatin1String(CD_PROFILE_METADATA_EDID_SERIAL)) {
        return i18n("Display serial number");
    }
    if (key == QLatin1String(CD_PROFILE_METADATA_EDID_MNFT)) {
        return i18n("Display PNPID");
    }
    if (key == QLatin1String(CD_PROFILE_METADATA_EDID_VENDOR)) {
        return i18n("Display vendor");
    }
    if (key == QLatin1String(CD_PROFILE_METADATA_FILE_CHECKSUM)) {
        return i18n("File checksum");
    }
    if (key == QLatin1String(CD_PROFILE_METADATA_CMF_PRODUCT)) {
        return i18n("Framework product");
    }
    if (key == QLatin1String(CD_PROFILE_METADATA_CMF_BINARY)) {
        return i18n("Framework program");
    }
    if (key == QLatin1String(CD_PROFILE_METADATA_CMF_VERSION)) {
        return i18n("Framework version");
    }
    if (key == QLatin1String(CD_PROFILE_METADATA_DATA_SOURCE)) {
        return i18n("Data source type");
    }
    if (key == QLatin1String(CD_PROFILE_METADATA_MAPPING_FORMAT)) {
        return i18n("Mapping format");
    }
    if (key == QLatin1String(CD_PROFILE_METADATA_MAPPING_QUALIFIER)) {
        return i18n("Mapping qualifier");
    }
    return key;
}
