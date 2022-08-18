/***************************************************************************
 *   Copyright (C) 2013-2016 by Daniel Nicoletti <dantti12@gmail.com>      *
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

#include <stdlib.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFileInfo>
#include <QStringBuilder>

#include <KAboutData>
#include <KLocalizedString>
#include <KMessageBox>

#include <version.h>

QString message(const QString &title, const QString &description, const QString &copyright)
{
    QString ret;
    ret = QLatin1String("<p><strong>") % title % QLatin1String("</strong></p>");
    if (!description.isEmpty()) {
        ret.append(i18n("Description: %1", description));
        if (copyright.isEmpty()) {
            return ret;
        }
    }

    if (!copyright.isEmpty()) {
        if (!description.isEmpty()) {
            ret.append(QLatin1String("<br>"));
        }
        ret.append(i18n("Copyright: %1", copyright));
    }

    return ret;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("application-vnd.iccprofile")));
    KAboutData about(QStringLiteral("colord-kde-icc-importer"),
                     i18n("ICC Profile Installer"),
                     COLORD_KDE_VERSION_STRING,
                     i18n("An application to install ICC profiles"),
                     KAboutLicense::GPL,
                     i18n("(C) 2008-2016 Daniel Nicoletti"));

    about.addAuthor(QStringLiteral("Daniel Nicoletti"), QString(), QStringLiteral("dantti12@gmail.com"), QStringLiteral("http://dantti.wordpress.com"));
    about.addCredit(QStringLiteral("Lukáš Tinkl"), i18n("Port to kf5"), QStringLiteral("ltinkl@redhat.com"));

    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(QStringLiteral("yes"), i18n("Do not prompt the user if he wants to install")));
    parser.addPositionalArgument("file", i18n("Color profile to install"), "+file");
    parser.process(app);
    about.processCommandLine(&parser);

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        parser.showHelp(EXIT_FAILURE);
    }

    QFileInfo fileInfo(args.first());
    // ~/.local/share/icc/
    const QString destFilename = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) % QLatin1String("/icc/") % fileInfo.fileName();

    Profile profile(fileInfo.filePath());
    if (!profile.loaded()) {
        KMessageBox::error(nullptr, i18n("Failed to parse file: %1", profile.errorMessage()), i18n("Failed to open ICC profile"));
        return 1;
    }
    qDebug() << fileInfo.filePath();
    qDebug() << destFilename;
    qDebug() << profile.checksum();

    if (QFile::exists(destFilename)) {
        KMessageBox::error(nullptr,
                           message(i18n("Color profile is already imported"), profile.description(), profile.copyright()),
                           i18n("ICC Profile Importer"));
        return 3;
    }

    CdInterface interface(QStringLiteral("org.freedesktop.ColorManager"), QStringLiteral("/org/freedesktop/ColorManager"), QDBusConnection::systemBus());
    QDBusReply<QDBusObjectPath> reply = interface.FindProfileById(profile.checksum());
    qDebug() << reply.error().message();
    if (reply.isValid() && reply.error().type() != QDBusError::NoError) {
        KMessageBox::error(nullptr,
                           message(i18n("ICC profile already installed system-wide"), profile.description(), profile.copyright()),
                           i18n("ICC Profile Importer"));
        return 3;
    }

    if (!parser.isSet("yes")) {
        const int ret = KMessageBox::questionYesNo(nullptr,
                                                   message(i18n("Would you like to import the color profile?"), profile.description(), profile.copyright()),
                                                   i18n("ICC Profile Importer"));
        if (ret == KMessageBox::No) {
            return 2;
        }
    }

    if (!QFile::copy(fileInfo.filePath(), destFilename)) {
        if (QFile::exists(destFilename)) {
            KMessageBox::error(nullptr, i18n("Failed to import color profile: color profile is already imported"), i18n("Importing Color Profile"));
            return 3;
        } else {
            KMessageBox::error(nullptr, i18n("Failed to import color profile: could not copy the file"), i18n("Importing Color Profile"));
            return 3;
        }
    }

    return EXIT_SUCCESS;
}
