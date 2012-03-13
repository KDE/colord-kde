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
#include "ProfileModel.h"
#include "ProfileDescription.h"

#include <KMessageBox>
#include <KGenericFactory>
#include <KAboutData>
#include <KTitleWidget>
#include <KIcon>

#include <QTimer>
#include <QVBoxLayout>
#include <QSortFilterProxyModel>

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
    ui->infoWidget->setPixmap(KTitleWidget::InfoMessage);
    m_addAction = ui->toolBar->addAction(KIcon("list-add"),
                                         i18nc("@action:intoolbar", "Add Profile"),
                                         this, SLOT(addProfile()));
    m_removeAction = ui->toolBar->addAction(KIcon("list-remove"),
                                            i18nc("@action:intoolbar", "Remove Profile"),
                                            this, SLOT(removeProfile()));

    // Devices view setup
    m_model = new DeviceModel(this);
    QSortFilterProxyModel *sortModel = new QSortFilterProxyModel(this);
    sortModel->setSourceModel(m_model);
    sortModel->setDynamicSortFilter(true);
    sortModel->setSortRole(DeviceModel::SortRole);
    ui->devicesTV->setModel(sortModel);
    ui->devicesTV->sortByColumn(0, Qt::AscendingOrder);
    connect(ui->devicesTV->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(showProfile()));
    connect(sortModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(showProfile()));

    // Profiles view setup
    ProfileModel *model = new ProfileModel(this);
    QSortFilterProxyModel *profileSortModel = new QSortFilterProxyModel(this);
    profileSortModel->setSourceModel(model);
    profileSortModel->setDynamicSortFilter(true);
    profileSortModel->setSortRole(ProfileModel::SortRole);
    ui->profilesTV->setModel(profileSortModel);
    ui->profilesTV->sortByColumn(0, Qt::AscendingOrder);
    connect(ui->profilesTV->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(showProfile()));
    connect(profileSortModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(showProfile()));
}

ColordKCM::~ColordKCM()
{
    delete ui;
}

void ColordKCM::update()
{

}

void ColordKCM::showProfile()
{
    QTreeView *view;
    if (ui->tabWidget->currentIndex() == 0) {
        view = ui->devicesTV;
    } else {
        view = ui->profilesTV;
    }

    if (view->model()->rowCount() == 0) {
        return;
    }

    QItemSelection selection;
    // we need to map the selection to source to get the real indexes
    selection = view->selectionModel()->selection();
    // select the first printer if there are profiles
    if (selection.indexes().isEmpty()) {
        view->selectionModel()->select(view->model()->index(0, 0),
                                               QItemSelectionModel::Select);
        return;
    }

    QModelIndex index = selection.indexes().first();
    ui->profile->setProfile(index.data(ProfileModel::ObjectPathRole).value<QDBusObjectPath>());
    if (ui->stackedWidget->currentWidget() != ui->profile) {
        ui->stackedWidget->setCurrentWidget(ui->profile);
    }
}

void ColordKCM::addProfile()
{
}

void ColordKCM::removeProfile()
{

}
