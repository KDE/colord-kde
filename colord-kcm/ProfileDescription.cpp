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

#include "ProfileDescription.h"
#include "ui_ProfileDescription.h"

#include "Profile.h"
#include "ProfileNamedColors.h"
#include "ProfileMetaData.h"

#include <math.h>

#include <QFileInfo>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <KDebug>

#define TAB_INFORMATION  1
#define TAB_CIE_1931     2
#define TAB_TRC          3
#define TAB_VCGT         4
#define TAB_FROM_SRGB    5
#define TAB_TO_SRGB      6
#define TAB_NAMED_COLORS 7
#define TAB_METADATA     8

typedef QMap<QString, QString>  StringStringMap;

ProfileDescription::ProfileDescription(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProfileDescription)
{
    ui->setupUi(this);

    m_namedColors = new ProfileNamedColors;
    m_metadata = new ProfileMetaData;
}

ProfileDescription::~ProfileDescription()
{
    delete m_namedColors;
    delete m_metadata;

    delete ui;
}

int ProfileDescription::innerHeight() const
{
    return ui->tabWidget->currentWidget()->sizeHint().height();
}

void ProfileDescription::setProfile(const QDBusObjectPath &objectPath)
{
    QDBusInterface *profileInterface;
    profileInterface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                         objectPath.path(),
                                         QLatin1String("org.freedesktop.ColorManager.Profile"),
                                         QDBusConnection::systemBus(),
                                         this);
    if (!profileInterface->isValid()) {
        profileInterface->deleteLater();
        return;
    }

    QString filename = profileInterface->property("Filename").toString();
    bool hasVcgt = profileInterface->property("HasVcgt").toBool();
    profileInterface->deleteLater();

    Profile profile(filename);
    if (profile.loaded()) {
        // Set the profile type
        ui->typeL->setText(profile.kindString());

        // Set the colorspace
        ui->colorspaceL->setText(profile.colorspace());

        // Set the version
        ui->versionL->setText(profile.version());

        // Set the created time
        ui->createdL->setText(profile.datetime().toString());

        // Set the license
        ui->licenseL->setText(profile.copyright());
        ui->licenseL->setVisible(!profile.copyright().isEmpty());
        ui->licenseLabel->setVisible(!profile.copyright().isEmpty());

        // Set the manufaturer
        ui->deviceMakeL->setText(profile.manufacturer());
        ui->deviceMakeL->setVisible(!profile.manufacturer().isEmpty());
        ui->makeLabel->setVisible(!profile.manufacturer().isEmpty());

        // Set the Model
        ui->deviceModelL->setText(profile.model());
        ui->deviceModelL->setVisible(!profile.model().isEmpty());
        ui->modelLabel->setVisible(!profile.model().isEmpty());

        // Set the Display Correction
        ui->dpCorrectionL->setText(hasVcgt ? i18n("Yes") : i18n("None"));

        // Set the file size
        ui->filesizeL->setText(KGlobal::locale()->formatByteSize(profile.size()));

        // Set the file name
        QFileInfo fileinfo(profile.filename());
        ui->filenameL->setText(fileinfo.fileName());

        QString temp;
        uint temperature = profile.temperature();
        if (fabs(temperature - 5000) < 10) {
            temp = QString::fromUtf8("%1ºK (D50)").arg(QString::number(temperature));
        } else if (fabs(temperature - 6500) < 10) {
            temp = QString::fromUtf8("%1ºK (D65)").arg(QString::number(temperature));
        } else {
            temp = QString::fromUtf8("%1ºK").arg(QString::number(temperature));
        }
        ui->whitepointL->setText(temp);

        kDebug() << profile.datetime();
        kDebug() << profile.description();
        kDebug() << profile.model();
        kDebug() << profile.manufacturer();
        kDebug() << profile.copyright();

        // Get named colors
        QMap<QString, QColor> namedColors = profile.getNamedColors();
        if (namedColors.size()) {
            m_namedColors->setNamedColors(namedColors);
            insertTab(TAB_NAMED_COLORS, m_namedColors, i18n("Named Colors"));
        } else {
            removeTab(m_namedColors);
        }

        // Get profile metadata
        QDBusMessage message;
        message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                                 objectPath.path(),
                                                 QLatin1String("org.freedesktop.DBus.Properties"),
                                                 QLatin1String("Get"));
        message << QString("org.freedesktop.ColorManager.Profile"); // Interface
        message << QString("Metadata"); // Propertie Name
        QDBusReply<QVariant> reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);

        StringStringMap metadata;
        if (reply.isValid()) {
            QDBusArgument argument = reply.value().value<QDBusArgument>();
            metadata = qdbus_cast<StringStringMap>(argument);
        }

        if (!metadata.isEmpty()) {
            m_metadata->setMetadata(metadata);
            insertTab(TAB_METADATA, m_metadata, i18n("Metadata"));
        } else {
            removeTab(m_metadata);
        }
    }
    kDebug() << profile.filename();
}

void ProfileDescription::insertTab(int index, QWidget *widget, const QString &label)
{
    int pos = ui->tabWidget->indexOf(widget);
    if (pos == -1) {
        ui->tabWidget->insertTab(index, widget, label);
    }
}

void ProfileDescription::removeTab(QWidget *widget)
{
    int pos = ui->tabWidget->indexOf(widget);
    if (pos != -1) {
        ui->tabWidget->removeTab(pos);
    }
}

#include "ProfileDescription.moc"
