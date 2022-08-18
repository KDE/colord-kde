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

#ifndef PROFILES_WATCHER_H
#define PROFILES_WATCHER_H

#include <KDirWatch>

#include <QFileInfo>
#include <QThread>
#include <QVariantList>

class Edid;
class ProfilesWatcher : public QThread
{
    Q_OBJECT
public:
    explicit ProfilesWatcher(QObject *parent = nullptr);

public Q_SLOTS:
    void scanHomeDirectory();
    void createIccProfile(bool isLaptop, const Edid &edid);

Q_SIGNALS:
    void scanFinished();

private Q_SLOTS:
    void addProfile(const QString &fileInfo);
    void removeProfile(const QString &filename);

private:
    QString profilesPath() const;

    KDirWatch *m_dirWatch = nullptr;
};

#endif // PROFILES_WATCHER_H
