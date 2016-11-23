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

//#include "CdInterface.h"

#include <QFileInfo>
#include <QIcon>
#include <QDebug>
#include <QGuiApplication>
#include <QCommandLineParser>

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(application);

    QGuiApplication app(argc, argv);

    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("application-vnd.iccprofile")));

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);

    return 0;
}
