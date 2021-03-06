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

#include <QApplication>

#include <consoleshell.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ConsoleShell *s = new ConsoleShell(&a);
    int returnCode;
    if (s->init(returnCode)) {
        return returnCode;
    }
    return a.exec();
}
