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

#include <math.h>

#include <QFileInfo>
#include <QDBusInterface>

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

        profile.getNamedColors();
    }
    kDebug() << profile.filename();
}

#include "ProfileDescription.moc"
