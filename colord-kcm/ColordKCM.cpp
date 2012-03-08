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

#include "ColordKCM.h"
#include "ui_ColordKCM.h"

#include "DeviceModel.h"
#include "ProfileDescription.h"

#include <KMessageBox>
#include <KGenericFactory>
#include <KAboutData>
#include <KTitleWidget>
#include <KIcon>

#include <QTimer>
#include <QVBoxLayout>

K_PLUGIN_FACTORY(ColordKCMFactory, registerPlugin<ColordKCM>();)
K_EXPORT_PLUGIN(ColordKCMFactory("kcm_colord"))

ColordKCM::ColordKCM(QWidget *parent, const QVariantList &args) :
    KCModule(ColordKCMFactory::componentData(), parent, args),
    ui(new Ui::ColordKCM)
{
    KAboutData *aboutData;
    aboutData = new KAboutData("kcm_colord",
                               "kcm_colord",
                               ki18n("Color settings"),
                               "0.1",
                               ki18n("Color settings"),
                               KAboutData::License_GPL,
                               ki18n("(C) 2012 Daniel Nicoletti"));
    setAboutData(aboutData);
    setButtons(NoAdditionalButton);

    ui->setupUi(this);

    m_addAction = ui->toolBar->addAction(KIcon("list-add"),
                                         i18nc("@action:intoolbar", "Add Profile"),
                                         this, SLOT(addProfile()));
    m_removeAction = ui->toolBar->addAction(KIcon("list-remove"),
                                            i18nc("@action:intoolbar", "Remove Profile"),
                                            this, SLOT(removeProfile()));

    m_model = new DeviceModel(this);
    ui->devicesTV->setModel(m_model);
    connect(ui->devicesTV->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(update()));
    connect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(update()));

    // Create the ProfileDescription before we try to select a printer
    m_profileDesc = new ProfileDescription(ui->scrollAreaWidgetContents);
    m_profileDesc->hide();

    // widget for when we don't have a printer
    m_noPrinter = new QWidget(ui->scrollAreaWidgetContents);
    KTitleWidget *widget = new KTitleWidget(m_noPrinter);
    widget->setText(i18n("You have no profiles"),
                         KTitleWidget::InfoMessage);
    widget->setComment(i18n("If you want to add one just click on the plus sign below the list"));

    QVBoxLayout *vertLayout = new QVBoxLayout(m_noPrinter);
    vertLayout->addStretch();
    vertLayout->addWidget(widget);
    vertLayout->addStretch();

    // if we get an error from the server we use this widget
    m_serverError = new QWidget(ui->scrollAreaWidgetContents);
    m_serverErrorW = new KTitleWidget(m_serverError);
    vertLayout = new QVBoxLayout(m_serverError);
    vertLayout->addStretch();
    vertLayout->addWidget(m_serverErrorW);
    vertLayout->addStretch();

    // the stacked layout allow us to chose which widget to show
    m_stackedLayout = new QStackedLayout(ui->scrollAreaWidgetContents);
    m_stackedLayout->addWidget(m_serverError);
    m_stackedLayout->addWidget(m_noPrinter);
    m_stackedLayout->addWidget(m_profileDesc);
    ui->scrollAreaWidgetContents->setLayout(m_stackedLayout);

    // select the first printer if there are printers
    if (m_model->rowCount()) {
        ui->devicesTV->selectionModel()->select(m_model->index(0, 0), QItemSelectionModel::Select);
    }
}

ColordKCM::~ColordKCM()
{
    delete ui;
}

void ColordKCM::update()
{

}

void ColordKCM::addProfile()
{
}

void ColordKCM::removeProfile()
{

}
