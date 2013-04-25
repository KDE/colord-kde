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

#include <config.h>

#include "DeviceModel.h"
#include "ProfileModel.h"
#include "Description.h"

#include "CdInterface.h"
#include "CdProfileInterface.h"
#include "CdDeviceInterface.h"

#include <KMessageBox>
#include <KGenericFactory>
#include <KAboutData>
#include <KFileDialog>
#include <KMimeType>
#include <KIcon>
#include <KUser>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusServiceWatcher>
#include <QItemSelectionModel>
#include <QTimer>
#include <QFileInfo>
#include <QStringBuilder>
#include <QSignalMapper>

#define DEVICE_PATH "device-path"

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
                               COLORD_KDE_VERSION,
                               ki18n("Color settings"),
                               KAboutData::License_GPL,
                               ki18n("(C) 2012 Daniel Nicoletti"));
    setAboutData(aboutData);
    setButtons(NoAdditionalButton);
    KGlobal::insertCatalog(QLatin1String("colord-kde"));

    ui->setupUi(this);
    ui->infoWidget->setPixmap(KTitleWidget::InfoMessage);
    connect(ui->addProfileBt, SIGNAL(clicked()), this, SLOT(addProfileFile()));

    m_addMenu = new QMenu(this);
    m_addMenu->addAction(KIcon("document-new"),
                         i18n("From File..."),
                         this, SLOT(addProfileFile()));

    m_addAvailableMenu = new QMenu(i18n("Available Profiles"), this);
    connect(m_addAvailableMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(addProfileAction(QAction*)));
    m_addMenu->addMenu(m_addAvailableMenu);
    ui->addProfileBt->setMenu(m_addMenu);
    ui->addProfileBt->setIcon(KIcon("list-add"));

    connect(m_addMenu, SIGNAL(aboutToShow()),
            this, SLOT(fillMenu()));

    connect(ui->tabWidget, SIGNAL(currentChanged(int)),
            this, SLOT(on_tabWidget_currentChanged(int)));

    ui->removeProfileBt->setIcon(KIcon("list-remove"));
    connect(ui->removeProfileBt, SIGNAL(clicked()),
            this, SLOT(removeProfile()));

    // Creates a ColorD interface, it must be created with new
    // otherwise the object will be deleted when this block ends
    m_cdInterface = new CdInterface(QLatin1String("org.freedesktop.ColorManager"),
                                    QLatin1String("/org/freedesktop/ColorManager"),
                                    QDBusConnection::systemBus(),
                                    this);
    ui->profile->setCdInterface(m_cdInterface);

    // listen to colord for events
    connect(m_cdInterface, SIGNAL(ProfileAdded(QDBusObjectPath)),
            this, SLOT(profileAdded(QDBusObjectPath)));

    // Devices view setup
    QSortFilterProxyModel *sortModel = new QSortFilterProxyModel(this);
    // Connect this slot prior to defining the model
    // so we get a selection on the first item for free
    connect(sortModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(showDescription()));
    sortModel->setDynamicSortFilter(true);
    sortModel->setSortRole(DeviceModel::SortRole);
    sortModel->sort(0);
    // Set the source model then connect to the selection model to get updates
    ui->devicesTV->setModel(sortModel);
    connect(ui->devicesTV->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(showDescription()));

    m_deviceModel = new DeviceModel(m_cdInterface, this);
    connect(m_deviceModel, SIGNAL(changed()), this, SLOT(updateSelection()));
    sortModel->setSourceModel(m_deviceModel);

    // Profiles view setup
    m_profileModel = new ProfileModel(this);
    connect(m_profileModel, SIGNAL(changed()), this, SLOT(updateSelection()));
    // Filter Proxy for the menu
    m_profilesFilter = new QSortFilterProxyModel(this);
    m_profilesFilter->setSourceModel(m_profileModel);
    m_profilesFilter->setFilterRole(ProfileModel::ColorspaceRole);
    m_profilesFilter->setSortRole(ProfileModel::SortRole);
    m_profilesFilter->setDynamicSortFilter(true);
    m_profilesFilter->sort(0);

    // Sort Proxy for the View
    QSortFilterProxyModel *profileSortModel = new QSortFilterProxyModel(this);
    profileSortModel->setSourceModel(m_profileModel);
    profileSortModel->setDynamicSortFilter(true);
    profileSortModel->setSortRole(ProfileModel::SortRole);
    profileSortModel->sort(0);
    ui->profilesTV->setModel(profileSortModel);
    connect(ui->profilesTV->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(showDescription()));
    connect(profileSortModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(showDescription()));


    // Make sure we know is colord is running
    QDBusServiceWatcher *watcher;
    watcher = new QDBusServiceWatcher("org.freedesktop.ColorManager",
                                      QDBusConnection::systemBus(),
                                      QDBusServiceWatcher::WatchForOwnerChange,
                                      this);
    connect(watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            m_deviceModel, SLOT(serviceOwnerChanged(QString,QString,QString)));
    connect(watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            m_profileModel, SLOT(serviceOwnerChanged(QString,QString,QString)));
    connect(watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            ui->profile, SLOT(serviceOwnerChanged(QString,QString,QString)));

    ui->devicesTb->setIcon(KIcon("preferences-activities"));
    ui->profilesTb->setIcon(KIcon("application-vnd.iccprofile"));

    QSignalMapper *signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(ui->devicesTb, 0);
    connect(ui->devicesTb, SIGNAL(clicked()), signalMapper, SLOT(map()));
    signalMapper->setMapping(ui->profilesTb, 1);
    connect(ui->profilesTb, SIGNAL(clicked()), signalMapper, SLOT(map()));

    connect(signalMapper, SIGNAL(mapped(int)),
            ui->tabWidget, SLOT(setCurrentIndex(int)));
    connect(signalMapper, SIGNAL(mapped(int)),
            this, SLOT(showDescription()));

    // make sure the screen is split on the half
    QList<int> sizes;
    sizes << width() / 2;
    sizes << width() / 2;
    ui->splitter->setSizes(sizes);
}

ColordKCM::~ColordKCM()
{
    delete ui;
}

void ColordKCM::load()
{
    // Force the profile widget to get a proper height in case
    // the stacked widget is showing the info page first
    if (ui->stackedWidget->currentWidget() != ui->profile_page) {
        // This is highly needed otherwise the size get wrong on System Settings
        ui->stackedWidget->setCurrentWidget(ui->profile_page);
    }
    ui->devicesTV->setFocus();

    // align the tabbar to the list view
    int offset = ui->profile->innerHeight() - ui->devicesTV->viewport()->height();
    ui->offsetSpacer->changeSize(30, offset, QSizePolicy::Fixed, QSizePolicy::Fixed);

    // Make sure we have something selected
    showDescription();
}

void ColordKCM::showDescription()
{
    QModelIndex index = currentIndex();
    if (!index.isValid()) {
        return;
    }

    if (index.data(DeviceModel::IsDeviceRole).toBool()) {
        ui->profile->setDevice(index.data(ProfileModel::ObjectPathRole).value<QDBusObjectPath>());
    } else {
        ui->profile->setProfile(index.data(ProfileModel::ObjectPathRole).value<QDBusObjectPath>());
    }

    if (ui->stackedWidget->currentWidget() != ui->profile_page) {
        ui->stackedWidget->setCurrentWidget(ui->profile_page);
    }

    // Check if we can remove the Profile
    bool enable;
    enable = index.data(ProfileModel::CanRemoveProfileRole).toBool();
    ui->removeProfileBt->setEnabled(enable);
}

void ColordKCM::addProfileFile()
{
    QModelIndex index = currentIndex();
    QString fileName;
    fileName = KFileDialog::getOpenFileName(KUrl(),
                                            QLatin1String("application/vnd.iccprofile"),
                                            this,
                                            i18n("Importing Color Profile"));
    if (fileName.isEmpty()) {
        return;
    }

    QFileInfo fileInfo(fileName);
    QString newFilename = profilesPath() % fileInfo.fileName();
    if (!QFile::copy(fileName, newFilename)) {
        KMessageBox::sorry(this, i18n("Failed to import color profile"), i18n("Importing Color Profile"));
    } else if (index.isValid()) {
        // Store the device kind and device object path
        // so that we assign the profile to the device when
        // ProfileAdded is emitted
        QString kind;
        QDBusObjectPath devicePath;
        kind = index.data(DeviceModel::ProfileKindRole).toString();
        devicePath = index.data(DeviceModel::ObjectPathRole).value<QDBusObjectPath>();
        m_profileFiles[newFilename] = KindAndPath(kind, devicePath);
    }
}

void ColordKCM::addProfileAction(QAction *action)
{
    QDBusObjectPath profileObject = action->data().value<QDBusObjectPath>();
    QDBusObjectPath deviceObject  = action->property(DEVICE_PATH).value<QDBusObjectPath>();

    addProvileToDevice(profileObject, deviceObject);
}

void ColordKCM::updateSelection()
{
    QAbstractItemView *view;
    if (sender() == m_deviceModel) {
        view = ui->devicesTV;
    } else {
        view = ui->profilesTV;
    }

    QItemSelection selection;
    selection = view->selectionModel()->selection();
    // Make sure we have an index selected
    if (selection.indexes().isEmpty()) {
        view->selectionModel()->select(view->model()->index(0, 0),
                                       QItemSelectionModel::SelectCurrent);
    }
}

void ColordKCM::removeProfile()
{
    QModelIndex index = currentIndex();
    if (!index.isValid()) {
        return;
    }

    int ret = KMessageBox::questionYesNo(this,
                                         i18n("Are you sure you want to remove this profile?"),
                                         i18n("Remove Profile"));
    if (ret == KMessageBox::No) {
        return;
    }

    if (index.parent().isValid()) {
        // If the item has a parent we are on the devices view
        QDBusObjectPath deviceObject;
        QDBusObjectPath profileObject;
        deviceObject  = index.data(ProfileModel::ParentObjectPathRole).value<QDBusObjectPath>();
        profileObject = index.data(ProfileModel::ObjectPathRole).value<QDBusObjectPath>();

        CdDeviceInterface device(QLatin1String("org.freedesktop.ColorManager"),
                                 deviceObject.path(),
                                 QDBusConnection::systemBus());
        if (device.isValid()) {
            device.RemoveProfile(profileObject);
        }
    } else {
        // We need to remove the file
        QString filename = index.data(ProfileModel::FilenameRole).toString();
        QFile file(filename);
        file.remove();
    }
}

void ColordKCM::fillMenu()
{
    m_addAvailableMenu->clear();
    QModelIndex index = currentIndex();
    if (!index.isValid()) {
        m_addAvailableMenu->setEnabled(false);
        return;
    }

    // if the user was selecting a profile get the parent
    if (index.parent().isValid()) {
        index = index.parent();
    }

    // Create a list of assigned profiles
    QList<QDBusObjectPath> assignedProfiles;
    int childCount = index.model()->rowCount(index);
    for (int i = 0; i < childCount; ++i) {
        assignedProfiles << index.child(i, 0).data(DeviceModel::ObjectPathRole).value<QDBusObjectPath>();
    }

    // Kind, colorspace must be matched
    QString kind = index.data(DeviceModel::ProfileKindRole).toString();
    QString colorspace = index.data(DeviceModel::ColorspaceRole).toString();
    QVariant devicePath = index.data(DeviceModel::ObjectPathRole);
    m_profilesFilter->setFilterFixedString(colorspace);
    for (int i = 0; i < m_profilesFilter->rowCount(); ++i) {
        QModelIndex profileIndex = m_profilesFilter->index(i, 0);

        // Check if the profile is of the same kind
        if (kind != profileIndex.data(ProfileModel::ProfileKindRole).toString()) {
            continue;
        }

        // Check if the profile isn't already assigned
        QDBusObjectPath profilePath;
        profilePath = profileIndex.data(ProfileModel::ObjectPathRole).value<QDBusObjectPath>();
        if (assignedProfiles.contains(profilePath)) {
            continue;
        }

        // Create the profile action
        QAction *action;
        QString title;
        title = profileIndex.data().toString();
        action = m_addAvailableMenu->addAction(title);
        action->setData(profileIndex.data(ProfileModel::ObjectPathRole));
        action->setProperty(DEVICE_PATH, devicePath);
    }

    // If the menu doesn't have any entries it should be disabled
    m_addAvailableMenu->setEnabled(!m_addAvailableMenu->actions().isEmpty());
}

void ColordKCM::on_tabWidget_currentChanged(int index)
{
    if (index == 0 && ui->addProfileBt->menu() == 0) {
        // adds the menu to the Add Profile button
        ui->addProfileBt->setMenu(m_addMenu);
    } else if (index) {
        // Remove the menu from the buttom since we can
        // only add files anyway
        ui->addProfileBt->setMenu(0);
    }
}

void ColordKCM::profileAdded(const QDBusObjectPath &objectPath)
{
    CdProfileInterface profile(QLatin1String("org.freedesktop.ColorManager"),
                               objectPath.path(),
                               QDBusConnection::systemBus());

    if (!profile.isValid()) {
        return;
    }

    QString kind = profile.kind();
    QString filename = profile.filename();

    if (m_profileFiles.contains(filename)) {
        if (m_profileFiles[filename].first != kind) {
            // The desired device did not match the profile kind
            KMessageBox::sorry(this,
                               i18n("Your profile did not match the device kind"),
                               i18n("Importing Color Profile"));
        } else {
            addProvileToDevice(objectPath, m_profileFiles[filename].second);
        }
        m_profileFiles.remove(filename);
    }
}

void ColordKCM::addProvileToDevice(const QDBusObjectPath &profile, const QDBusObjectPath &devicePath) const
{
    CdDeviceInterface device(QLatin1String("org.freedesktop.ColorManager"),
                             devicePath.path(),
                             QDBusConnection::systemBus());
    if (device.isValid()) {
        device.AddProfile(QLatin1String("hard"), profile);
    }
}

QModelIndex ColordKCM::currentIndex() const
{
    QModelIndex ret;
    QAbstractItemView *view;
    if (ui->tabWidget->currentIndex() == 0) {
        view = ui->devicesTV;
    } else {
        view = ui->profilesTV;
    }

    if (view->model()->rowCount() == 0) {
        if (ui->stackedWidget->currentWidget() != ui->info_page) {
            ui->stackedWidget->setCurrentWidget(ui->info_page);
        }

        if (ui->tabWidget->currentIndex() == 0) {
            // Devices is empty
            ui->infoWidget->setText(i18n("You do not have any devices registered"));
            ui->infoWidget->setComment(i18n("Make sure colord module on kded is running"));
        } else {
            // Profiles is empty
            ui->infoWidget->setText(i18n("You do not have any profiles registered"));
            ui->infoWidget->setComment(i18n("Add one by clicking Add Profile button"));
        }

        return ret;
    }

    QItemSelection selection;
    selection = view->selectionModel()->selection();
    // select the first index if the selection is not empty
    if (!selection.indexes().isEmpty()) {
        ret = selection.indexes().first();
    }

    return ret;
}

QString ColordKCM::profilesPath() const
{
    KUser user;
    // ~/.local/share/icc/
    return user.homeDir() % QLatin1String("/.local/share/icc/");
}
