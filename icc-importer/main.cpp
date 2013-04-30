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
#include "CdInterface.h"

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

QString message(const QString &title, const QString &description, const QString &copyright)
{
    QString ret;
    ret = QLatin1String("<p><strong>") % title % QLatin1String("</strong></p>");
    if (!description.isNull()) {
        ret.append(i18n("Description: %1", description));
        if (copyright.isNull()) {
            return ret;
        }
    }

    if (!copyright.isNull()) {
        if (!description.isNull()) {
            ret.append(QLatin1String("<br>"));
        }
        ret.append(i18n("Copyright: %1", copyright));
    }

    return ret;
}

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
    options.add("yes", ki18n("Do not prompt the user if he want to install"));
    options.add("+file", ki18n("Color profile to install"));
    KCmdLineArgs::addCmdLineOptions(options);

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    KApplication app;

    KUser user;
    QFileInfo fileInfo(args->url(0).toLocalFile());
    // ~/.local/share/icc/
    QString destFilename = user.homeDir() % QLatin1String("/.local/share/icc/") % fileInfo.fileName();

    Profile profile(fileInfo.filePath());
    if (!profile.loaded()) {
        KMessageBox::error(0,
                           i18n("Failed to parse file: %1", profile.errorMessage()),
                           i18n("Failed to open ICC profile"));
        return 1;
    }
    kDebug() << fileInfo.filePath();
    kDebug() << destFilename;
    kDebug() << profile.checksum();

    if (QFile::exists(destFilename)) {
        KMessageBox::sorry(0,
                           message(i18n("Color profile is already imported"),
                                   profile.description(),
                                   profile.copyright()),
                           i18n("ICC Profile Importer"));
        return 3;
    }

    CdInterface interface(QLatin1String("org.freedesktop.ColorManager"),
                          QLatin1String("/org/freedesktop/ColorManager"),
                          QDBusConnection::systemBus());
    QDBusReply<QDBusObjectPath> reply = interface.FindProfileById(profile.checksum());
    kDebug() << reply.error().message();
    if (reply.isValid() && reply.error().type() != QDBusError::NoError) {
        KMessageBox::sorry(0,
                           message(i18n("ICC profile already installed system-wide"),
                                   profile.description(),
                                   profile.copyright()),
                           i18n("ICC Profile Importer"));
        return 3;
    }

    if (!args->isSet("yes")) {
        int ret;
        ret = KMessageBox::questionYesNo(0,
                                         message(i18n("Would you like to import the color profile?"),
                                                 profile.description(),
                                                 profile.copyright()),
                                         i18n("ICC Profile Importer"));
        if (ret == KMessageBox::No) {
            return 2;
        }
    }

    if (!QFile::copy(fileInfo.filePath(), destFilename)) {
        if (QFile::exists(destFilename)) {
            KMessageBox::error(0,
                               i18n("Failed to import color profile: color profile is already imported"),
                               i18n("Importing Color Profile"));
            return 3;
        } else {
            KMessageBox::sorry(0,
                               i18n("Failed to import color profile: could not copy the file"),
                               i18n("Importing Color Profile"));
            return 3;
        }
    }

    return 0;
}
