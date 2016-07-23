/*
 * Copyright 2010 and beyond, Juan Luis Paz
 *
 * This file is part of Script Report.
 *
 * Script Report is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Script Report is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Script Report.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QTranslator>
#include <QLibraryInfo>
#include <QLocale>
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(MainWindow::tr("Script Report Editor"));
    a.setApplicationVersion(QString::fromLatin1(APP_VERSION));

    QTranslator qtTranslator;
    qtTranslator.load(QString::fromLatin1("qt_") + QLocale::system().name(),
            QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qtTranslator);

    QTranslator sreTranslator;
    sreTranslator.load(QString::fromLatin1("translations/scriptreporteditor_") + QLocale::system().name());
    a.installTranslator(&sreTranslator);

    MainWindow w;
    w.show();
    return a.exec();
}
