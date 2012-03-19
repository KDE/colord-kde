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
#include <KFileDialog>
#include <KMimeType>
#include <KIcon>
#include <KUser>

#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QTimer>
#include <QToolButton>
#include <QFileInfo>
#include <QVBoxLayout>
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
                               "0.1",
                               ki18n("Color settings"),
                               KAboutData::License_GPL,
                               ki18n("(C) 2012 Daniel Nicoletti"));
    setAboutData(aboutData);
    setButtons(NoAdditionalButton);

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
    // Filter Proxy for the menu
    m_profilesFilter = new QSortFilterProxyModel(this);
    m_profilesFilter->setSourceModel(model);
    m_profilesFilter->setFilterRole(ProfileModel::ColorspaceRole);
    m_profilesFilter->setSortRole(ProfileModel::ProfileDisplayNameSourceRole);
    m_profilesFilter->setDynamicSortFilter(true);
    m_profilesFilter->sort(0);

    // Sort Proxy for the View
    QSortFilterProxyModel *profileSortModel = new QSortFilterProxyModel(this);
    profileSortModel->setSourceModel(model);
    profileSortModel->setDynamicSortFilter(true);
    profileSortModel->setSortRole(ProfileModel::SortRole);
    profileSortModel->sort(0, Qt::AscendingOrder);
    ui->profilesTV->setModel(profileSortModel);
    connect(ui->profilesTV->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(showProfile()));
    connect(profileSortModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(showProfile()));

    // Creates a ColorD interface, it must be created with new
    // otherwise the object will be deleted when this block ends
    QDBusInterface *interface;
    interface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                   QLatin1String("/org/freedesktop/ColorManager"),
                                   QLatin1String("org.freedesktop.ColorManager"),
                                   QDBusConnection::systemBus(),
                                   this);

    // listen to colord for events
    connect(interface, SIGNAL(ProfileAdded(QDBusObjectPath)),
            this, SLOT(profileAdded(QDBusObjectPath)));

    ui->devicesTb->setIcon(KIcon("preferences-activities"));
    ui->profilesTb->setIcon(KIcon("application-vnd.iccprofile"));

    QSignalMapper *signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(ui->devicesTb, 0);
    connect(ui->devicesTb, SIGNAL(clicked()), signalMapper, SLOT(map()));
    signalMapper->setMapping(ui->profilesTb, 1);
    connect(ui->profilesTb, SIGNAL(clicked()), signalMapper, SLOT(map()));

    connect(signalMapper, SIGNAL(mapped(int)),
            ui->tabWidget, SLOT(setCurrentIndex(int)));

    // align the tabbar to the list view
    int offset = ui->profile->innerHeight() - ui->devicesTV->sizeHint().height();
    ui->offsetSpacer->changeSize(30, offset, QSizePolicy::Fixed, QSizePolicy::Fixed);

    QList<int> sizes;
    sizes << width() / 2;
    sizes << width() / 2;
    ui->splitter->setSizes(sizes);
}

ColordKCM::~ColordKCM()
{
    delete ui;
}

void ColordKCM::showProfile()
{
    QModelIndex index = currentIndex();
    if (!index.isValid()) {
        return;
    }

    ui->profile->setProfile(index.data(ProfileModel::ObjectPathRole).value<QDBusObjectPath>());
    if (ui->stackedWidget->currentWidget() != ui->profile) {
        ui->stackedWidget->setCurrentWidget(ui->profile);
    }

    // Check if we can remove the Profile
    bool enable = false;
    if (ui->tabWidget->currentIndex() == 1) {
        QString filename = index.data(ProfileModel::FilenameRole).toString();
        QFileInfo fileInfo(filename);
        kDebug() << "---" << filename << fileInfo.isWritable();
        if (!filename.isNull() && fileInfo.isWritable()) {
            enable = true;
        }
    } else {
        // It's ok to remove profiles from devices
        if (index.parent().isValid()) {
            enable = true;
        }
    }
    ui->removeProfileBt->setEnabled(enable);
}

void ColordKCM::addProfileFile()
{
    QModelIndex index = currentIndex();
    if (!index.isValid()) {
        return;
    }

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
    kDebug() << fileName << newFilename;
    if (!QFile::copy(fileName, newFilename)) {
        KMessageBox::sorry(this, i18n("Failed to import color profile"), i18n("Importing Color Profile"));
    } else {
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

        QDBusMessage message;
        message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                                 deviceObject.path(),
                                                 QLatin1String("org.freedesktop.ColorManager.Device"),
                                                 QLatin1String("RemoveProfile"));
        message << qVariantFromValue(profileObject); // Profile Path

        /* call Device.RemoveProfile() with the device and profile object paths */
        QDBusConnection::systemBus().send(message);
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
        title = profileIndex.data(ProfileModel::ProfileDisplayNameSourceRole).toString();
        action = m_addAvailableMenu->addAction(title);
        action->setData(profileIndex.data(ProfileModel::ObjectPathRole));
        action->setProperty(DEVICE_PATH, devicePath);
    }
    m_addAvailableMenu->setEnabled(m_profilesFilter->rowCount());
}

void ColordKCM::on_tabWidget_currentChanged(int index)
{
    kDebug() << index;
    if (index == 0 && ui->addProfileBt->menu() == 0) {
        ui->addProfileBt->setMenu(m_addMenu);
        kDebug() << "add";
    } else if (index) {
        kDebug() << "remove";
        ui->addProfileBt->setMenu(0);
    }
}

void ColordKCM::profileAdded(const QDBusObjectPath &objectPath)
{
    QDBusInterface *interface;
    interface = new QDBusInterface(QLatin1String("org.freedesktop.ColorManager"),
                                   objectPath.path(),
                                   QLatin1String("org.freedesktop.ColorManager.Profile"),
                                   QDBusConnection::systemBus(),
                                   this);
    interface->deleteLater();
    if (!interface->isValid()) {
        return;
    }

    QString kind = interface->property("Kind").toString();
    QString filename = interface->property("Filename").toString();

    if (m_profileFiles.contains(filename)) {
        QString deviceKind = m_profileFiles[filename].first % QLatin1String("-device");
        if (deviceKind != kind) {
            // The desired device did not match the profile kind
            kDebug() << deviceKind << kind << filename;
            KMessageBox::sorry(this, i18n("Your profile did not match the device kind"), i18n("Importing Color Profile"));
        } else {
            addProvileToDevice(objectPath, m_profileFiles[filename].second);
        }
        m_profileFiles.remove(filename);
    }
}

void ColordKCM::addProvileToDevice(const QDBusObjectPath &profilePath, const QDBusObjectPath &devicePath) const
{
    QDBusMessage message;
    message = QDBusMessage::createMethodCall(QLatin1String("org.freedesktop.ColorManager"),
                                             devicePath.path(),
                                             QLatin1String("org.freedesktop.ColorManager.Device"),
                                             QLatin1String("AddProfile"));
    message << QString("hard"); // Relation
    message << qVariantFromValue(profilePath); // Profile Path

    /* call Device.AddProfile() with the device and profile object paths */
    QDBusConnection::systemBus().send(message);
}

QModelIndex ColordKCM::currentIndex() const
{
    QAbstractItemView *view;
    if (ui->tabWidget->currentIndex() == 0) {
        view = ui->devicesTV;
    } else {
        view = ui->profilesTV;
    }

    if (view->model()->rowCount() == 0) {
        return QModelIndex();
    }

    QItemSelection selection;
    // we need to map the selection to source to get the real indexes
    selection = view->selectionModel()->selection();
    // select the first printer if there are profiles
    if (selection.indexes().isEmpty()) {
        view->selectionModel()->select(view->model()->index(0, 0),
                                       QItemSelectionModel::Select);
        return QModelIndex();
    }

    return selection.indexes().first();
}

QString ColordKCM::profilesPath() const
{
    KUser user;
    // ~/.local/share/icc/
    return user.homeDir() % QLatin1String("/.local/share/icc/");
}
