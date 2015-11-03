/*
    Elina Application Server for Scanner Inventory Management
    Copyright (C) 2012 Dimitris Paraschou

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include <QtGui/QApplication>
#include "dialog.h"
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog w;
    QFile st(QDir::currentPath()+ "/settings.ini");
    if(!st.open(QIODevice::ReadOnly | QIODevice::Text))

    {

        QString settingsFile = (QDir::currentPath()+ "/settings.ini");
        QSettings *settings =new QSettings(settingsFile,QSettings::IniFormat);

        settings->setValue("host", "194.111.212.84");
        settings->setValue("dbuser", "sa");
        settings->setValue("dbpassword", "sa");
        settings->setValue("hfsserver", "c://jim//hfs//upload//");
        settings->setValue("apath","C://Program Files//algo//");
        settings->setValue("version","1.1");
        settings->sync();
        delete (settings);
    }
    st.close();


    //w.show();
    return a.exec();
}
