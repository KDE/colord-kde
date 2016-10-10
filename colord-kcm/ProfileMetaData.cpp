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

#include "ProfileMetaData.h"
#include "ui_ProfileMetaData.h"

#include <QDebug>

/* defined in metadata-spec.txt */
#define CD_PROFILE_METADATA_STANDARD_SPACE	QStringLiteral("STANDARD_space")
#define CD_PROFILE_METADATA_EDID_MD5		QStringLiteral("EDID_md5")
#define CD_PROFILE_METADATA_EDID_MODEL		QStringLiteral("EDID_model")
#define CD_PROFILE_METADATA_EDID_SERIAL		QStringLiteral("EDID_serial")
#define CD_PROFILE_METADATA_EDID_MNFT		QStringLiteral("EDID_mnft")
#define CD_PROFILE_METADATA_EDID_VENDOR		QStringLiteral("EDID_manufacturer")
#define CD_PROFILE_METADATA_FILE_CHECKSUM	QStringLiteral("FILE_checksum")
#define CD_PROFILE_METADATA_CMF_PRODUCT		QStringLiteral("CMF_product")
#define CD_PROFILE_METADATA_CMF_BINARY		QStringLiteral("CMF_binary")
#define CD_PROFILE_METADATA_CMF_VERSION		QStringLiteral("CMF_version")
#define CD_PROFILE_METADATA_DATA_SOURCE		QStringLiteral("DATA_source")
#define CD_PROFILE_METADATA_MAPPING_FORMAT	QStringLiteral("MAPPING_format")
#define CD_PROFILE_METADATA_MAPPING_QUALIFIER	QStringLiteral("MAPPING_qualifier")

ProfileMetaData::ProfileMetaData(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProfileMetaData),
    m_model(new QStandardItemModel(this))
{
    ui->setupUi(this);

    m_model->setColumnCount(2);
    ui->treeView->setModel(m_model);
    ui->treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
}

ProfileMetaData::~ProfileMetaData()
{
    delete ui;
}

void ProfileMetaData::setMetadata(const CdStringMap &metadata)
{
    m_model->removeRows(0, m_model->rowCount());

    CdStringMap::const_iterator i = metadata.constBegin();
    while (i != metadata.constEnd()) {
        qDebug() << i.key() << ": " << i.value();
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
