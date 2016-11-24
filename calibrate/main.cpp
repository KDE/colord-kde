/***************************************************************************
 *   Copyright (C) 2016 by Daniel Nicoletti <dantti12@gmail.com>           *
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
#include "colordhelper.h"

#include <QFileInfo>
#include <QIcon>
#include <QDebug>
#include <QGuiApplication>
#include <QCommandLineParser>

#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <iostream>

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(application);

    QGuiApplication app(argc, argv);

    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("application-vnd.iccprofile")));

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();

    QCommandLineOption showDaemonVersionOption("daemon-version",
                                               QCoreApplication::translate("main", "Prints the colord-session daemon version."));

    QCommandLineOption deviceIdOption("device",
                                      QCoreApplication::translate("main", "Set the specific device to calibrate."),
                                      QCoreApplication::translate("main", "device"));
    parser.addOptions({
                          showDaemonVersionOption,
                          deviceIdOption
                      });

    parser.process(app);

    if (parser.isSet(showDaemonVersionOption)) {
        std::cout << ColordHelper::daemonVersion().toLatin1().constData() << std::endl;
        return 0;
    }

    if (!parser.isSet(deviceIdOption)) {
        std::cout << QCoreApplication::translate("main", "No device was specified!").toUtf8().constData() << std::endl;
        return 1;
    }

    ColordHelper helper(parser.value(deviceIdOption));

    QQmlApplicationEngine *engine = new QQmlApplicationEngine;
    engine->rootContext()->setContextProperty("ColordHelper", &helper);
    engine->load(QUrl(QLatin1String("qrc:/main.qml")));

    return app.exec();
}
