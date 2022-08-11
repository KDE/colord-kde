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

#ifndef COLORD_KCM_H
#define COLORD_KCM_H

#include <KCModule>
#include <KTitleWidget>

#include <QMenu>
#include <QStackedLayout>
#include <QSortFilterProxyModel>
#include <QDBusObjectPath>

typedef QPair<QString, QDBusObjectPath> KindAndPath;

namespace Ui {
    class ColordKCM;
}
class CdInterface;
class DeviceModel;
class ProfileModel;
class ProfileDescription;

class ColordKCM : public KCModule
{
    Q_OBJECT
public:
    ColordKCM(QWidget *parent, const QVariantList &args);
    ~ColordKCM() override;

public slots:
    void load() override;

private slots:
    void showDescription();
    void addProfileFile();
    void addProfileAction(QAction *action);
    void updateSelection();
    void removeProfile();
    void fillMenu();
    void on_tabWidget_currentChanged(int index);
    void profileAdded(const QDBusObjectPath &objectPath);

private:
    void addProvileToDevice(const QDBusObjectPath &profile, const QDBusObjectPath &devicePath) const;
    QModelIndex currentIndex() const;
    QString profilesPath() const;

    Ui::ColordKCM *ui;
    DeviceModel *m_deviceModel;
    ProfileModel *m_profileModel;
    QStackedLayout *m_stackedLayout;
    ProfileDescription *m_profileDesc;
    QWidget *m_noPrinter;
    QWidget *m_serverError;
    KTitleWidget *m_serverErrorW;
    int m_lastError;
    QMenu *m_addMenu;
    QMenu *m_addAvailableMenu;
    QSortFilterProxyModel *m_profilesFilter;
    QHash<QString, KindAndPath> m_profileFiles;

    QAction *m_addAction;
    QAction *m_removeAction;
    QAction *m_configureAction;
    CdInterface *m_cdInterface;
};

#endif // COLORD_KCM_H
