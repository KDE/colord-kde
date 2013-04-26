/***************************************************************************
 *   Copyright (C) 2013 by Daniel Nicoletti                                *
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

#include "../colord-kcm/Profile.h"

#include <config.h>

#include <QFileInfo>
#include <QStringBuilder>

#include <KUrl>
#include <KUser>
#include <KMessageBox>
#include <KApplication>
#include <KConfig>
#include <KAboutData>
#include <KCmdLineArgs>

#include <KDebug>

int main(int argc, char **argv)
{
    KAboutData about("colord-kde-icc-importer",
                     "colord-kde",
                     ki18n("Apper"),
                     COLORD_KDE_VERSION,
                     ki18n("Apper is an Application to Get and Manage Software"),
                     KAboutData::License_GPL,
                     ki18n("(C) 2008-2013 Daniel Nicoletti"));

    about.addAuthor(ki18n("Daniel Nicoletti"), KLocalizedString(), "dantti12@gmail.com", "http://dantti.wordpress.com");
    about.setProgramIconName("application-vnd.iccprofile");

    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("+file", ki18n("Color profile to install"));
    KCmdLineArgs::addCmdLineOptions(options);

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if (args->count() == 0) {
        KCmdLineArgs::usage();
    }

    KApplication app;

    KUser user;
    // ~/.local/share/icc/
    QString profilesPath = user.homeDir() % QLatin1String("/.local/share/icc/");

    // grab the list of files
    for (int i = 0; i < args->count(); i++) {
        QFileInfo fileInfo(args->url(i).toLocalFile());
        Profile profile(fileInfo.filePath());
        if (!profile.loaded()) {
            KMessageBox::sorry(0,
                               i18n("Failed to read color profile"),
                               i18n("Import Color Profile"));
            return 1;
        }
        kDebug() << fileInfo.filePath();

        QString text;
        text.append(QLatin1String("<p><strong>"));
        text.append(i18n("Would you like to import the color profile?"));
        text.append(QLatin1String("</strong></p>"));
        text.append(i18n("Description: %1", profile.description()));
        text.append(QLatin1String("<br>"));
        text.append(i18n("Copyright: %1", profile.copyright()));
        int ret;
        ret = KMessageBox::questionYesNo(0,
                                         text,
                                         i18n("Import Color Profile"));
        if (ret == KMessageBox::No) {
            return 2;
        }

        QString newFilename = profilesPath % fileInfo.filePath();
        if (!QFile::copy(fileInfo.filePath(), newFilename)) {
            if (QFile::exists(newFilename)) {
                KMessageBox::error(0,
                                   i18n("Failed to import color profile: file already exists"),
                                   i18n("Importing Color Profile"));
            } else {
                KMessageBox::sorry(0,
                                   i18n("Failed to import color profile: could not copy the file"),
                                   i18n("Importing Color Profile"));
            }
        }
    }

    return 0;
}
