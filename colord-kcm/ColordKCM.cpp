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

#include "ColordKCM.h"
#include "ui_ColordKCM.h"

#include <config.h>

#include "DeviceModel.h"
#include "ProfileModel.h"
#include "Description.h"
#include "NoSelectionRectDelegate.h"

#include "CdInterface.h"
#include "CdProfileInterface.h"
#include "CdDeviceInterface.h"

#include <KMessageBox>
#include <KPluginFactory>
#include <KAboutData>

#include <QProcess>
#include <QFileDialog>
#include <QItemSelectionModel>
#include <QTimer>
#include <QFileInfo>
#include <QStringBuilder>
#include <QSignalMapper>
#include <QIcon>

#define DEVICE_PATH "device-path"

K_PLUGIN_FACTORY(ColordKCMFactory, registerPlugin<ColordKCM>();)

ColordKCM::ColordKCM(QWidget *parent, const QVariantList &args) :
    KCModule(parent, args),
    ui(new Ui::ColordKCM),
    m_addMenu(new QMenu(this)),
    m_addAvailableMenu(new QMenu(i18n("Available Profiles"), this))
{
    auto aboutData = new KAboutData(QStringLiteral("kcm_colord"),
                                    i18n("Color settings"),
                                    QStringLiteral(COLORD_KDE_VERSION),
                                    i18n("Color settings"),
                                    KAboutLicense::GPL,
                                    i18n("(C) 2012-2016 Daniel Nicoletti"));
    aboutData->addCredit(QStringLiteral("Lukáš Tinkl"),
                         i18n("Port to kf5"),
                         QStringLiteral("ltinkl@redhat.com"));
    setAboutData(aboutData);
    setButtons(NoAdditionalButton);

    ui->setupUi(this);
    ui->infoWidget->setPixmap(KTitleWidget::InfoMessage);
    connect(ui->addProfileBt, &QToolButton::clicked, this, &ColordKCM::addProfileFile);

    m_addMenu->addAction(QIcon::fromTheme(QStringLiteral("document-new")),
                         i18n("From File..."),
                         this, SLOT(addProfileFile()));

    connect(m_addAvailableMenu, &QMenu::triggered,
            this, &ColordKCM::addProfileAction);
    m_addMenu->addMenu(m_addAvailableMenu);
    ui->addProfileBt->setMenu(m_addMenu);
    ui->addProfileBt->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));

    connect(m_addMenu, &QMenu::aboutToShow,
            this, &ColordKCM::fillMenu);

    connect(ui->tabWidget, &QStackedWidget::currentChanged,
            this, &ColordKCM::on_tabWidget_currentChanged);

    ui->removeProfileBt->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    connect(ui->removeProfileBt, &QToolButton::clicked,
            this, &ColordKCM::removeProfile);

    // Creates a ColorD interface, it must be created with new
    // otherwise the object will be deleted when this block ends
    m_cdInterface = new CdInterface(QStringLiteral("org.freedesktop.ColorManager"),
                                    QStringLiteral("/org/freedesktop/ColorManager"),
                                    QDBusConnection::systemBus(),
                                    this);
    ui->profile->setCdInterface(m_cdInterface);

    // listen to colord for events
    connect(m_cdInterface, &CdInterface::ProfileAdded,
            this, &ColordKCM::profileAdded);

    // Devices view setup
    auto sortModel = new QSortFilterProxyModel(this);

    // Connect this slot prior to defining the model
    // so we get a selection on the first item for free
    connect(sortModel, &QSortFilterProxyModel::dataChanged,
            this, &ColordKCM::showDescription);
    sortModel->setDynamicSortFilter(true);
    sortModel->setSortRole(DeviceModel::SortRole);
    sortModel->sort(0);
    // Set the source model then connect to the selection model to get updates
    ui->devicesTV->setModel(sortModel);
    ui->devicesTV->setItemDelegate(new NoSelectionRectDelegate(this));
    connect(ui->devicesTV->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ColordKCM::showDescription);

    m_deviceModel = new DeviceModel(m_cdInterface, this);
    connect(m_deviceModel, &DeviceModel::changed, this, &ColordKCM::updateSelection);
    sortModel->setSourceModel(m_deviceModel);

    // Profiles view setup
    m_profileModel = new ProfileModel(m_cdInterface, this);
    connect(m_profileModel, &ProfileModel::changed, this, &ColordKCM::updateSelection);
    // Filter Proxy for the menu
    m_profilesFilter = new QSortFilterProxyModel(this);
    m_profilesFilter->setSourceModel(m_profileModel);
    m_profilesFilter->setFilterRole(ProfileModel::ColorspaceRole);
    m_profilesFilter->setSortRole(ProfileModel::SortRole);
    m_profilesFilter->setDynamicSortFilter(true);
    m_profilesFilter->sort(0);

    // Sort Proxy for the View
    auto profileSortModel = new QSortFilterProxyModel(this);
    profileSortModel->setSourceModel(m_profileModel);
    profileSortModel->setDynamicSortFilter(true);
    profileSortModel->setSortRole(ProfileModel::SortRole);
    profileSortModel->sort(0);
    ui->profilesTV->setModel(profileSortModel);
    ui->profilesTV->setItemDelegate(new NoSelectionRectDelegate(this));
    connect(ui->profilesTV->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ColordKCM::showDescription);
    connect(profileSortModel, &QSortFilterProxyModel::dataChanged,
            this, &ColordKCM::showDescription);

    // Make sure we know is colord is running
    auto watcher = new QDBusServiceWatcher(QStringLiteral("org.freedesktop.ColorManager"),
                                           QDBusConnection::systemBus(),
                                           QDBusServiceWatcher::WatchForOwnerChange,
                                           this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged,
            m_deviceModel, &DeviceModel::serviceOwnerChanged);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged,
            m_profileModel, &ProfileModel::serviceOwnerChanged);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged,
            ui->profile, &Description::serviceOwnerChanged);

    ui->devicesTb->setIcon(QIcon::fromTheme(QStringLiteral("computer")));
    ui->profilesTb->setIcon(QIcon::fromTheme(QStringLiteral("application-vnd.iccprofile")));

    auto signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(ui->devicesTb, 0);
    connect(ui->devicesTb, SIGNAL(clicked()), signalMapper, SLOT(map()));
    signalMapper->setMapping(ui->profilesTb, 1);
    connect(ui->profilesTb, SIGNAL(clicked()), signalMapper, SLOT(map()));

    connect(signalMapper, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            ui->tabWidget, &QStackedWidget::setCurrentIndex);
    connect(signalMapper, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &ColordKCM::showDescription);

    // make sure the screen is split on the half
    ui->splitter->setSizes({ width() / 2, width() / 2 });
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

    bool canRemoveProfile = index.data(ProfileModel::CanRemoveProfileRole).toBool();
    if (index.data(DeviceModel::IsDeviceRole).toBool()) {
        ui->profile->setDevice(index.data(ProfileModel::ObjectPathRole).value<QDBusObjectPath>());
    } else {
        ui->profile->setProfile(index.data(ProfileModel::ObjectPathRole).value<QDBusObjectPath>(),
                                canRemoveProfile);
    }
    ui->removeProfileBt->setEnabled(canRemoveProfile);

    if (ui->stackedWidget->currentWidget() != ui->profile_page) {
        ui->stackedWidget->setCurrentWidget(ui->profile_page);
    }
}

void ColordKCM::addProfileFile()
{
    QModelIndex index = currentIndex();
    QFileDialog dlg(this, i18n("Importing Color Profile"));
    dlg.setMimeTypeFilters({QStringLiteral("application/vnd.iccprofile")});
    if (dlg.exec() != QFileDialog::Accepted) {
        return;
    }

    const QString fileName = dlg.selectedFiles().first();

    // Store the device kind and device object path
    // so that we assign the profile to the device when
    // ProfileAdded is emitted
    QFileInfo fileInfo(fileName);
    QString kind = index.data(DeviceModel::ProfileKindRole).toString();
    QString newFilename = profilesPath() % fileInfo.fileName();
    QDBusObjectPath devicePath;
    devicePath = index.data(DeviceModel::ObjectPathRole).value<QDBusObjectPath>();
    m_profileFiles[newFilename] = KindAndPath(kind, devicePath);

    int ret = QProcess::execute(QStringLiteral("colord-kde-icc-importer"), {QStringLiteral("--yes"), fileName});
    if (ret != EXIT_SUCCESS) {
        m_profileFiles.remove(newFilename);
    }
}

void ColordKCM::addProfileAction(QAction *action)
{
    auto profileObject = action->data().value<QDBusObjectPath>();
    auto deviceObject  = action->property(DEVICE_PATH).value<QDBusObjectPath>();

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
        auto deviceObject  = index.data(ProfileModel::ParentObjectPathRole).value<QDBusObjectPath>();
        auto profileObject = index.data(ProfileModel::ObjectPathRole).value<QDBusObjectPath>();

        CdDeviceInterface device(QStringLiteral("org.freedesktop.ColorManager"),
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
    CdProfileInterface profile(QStringLiteral("org.freedesktop.ColorManager"),
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
    CdDeviceInterface device(QStringLiteral("org.freedesktop.ColorManager"),
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
    // ~/.local/share/icc/
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) % QLatin1String("/icc/");
}

#include "ColordKCM.moc"
