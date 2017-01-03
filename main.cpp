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

#include <QtWidgets/QApplication>
#include "dialog.h"
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog w;
    QFile st(QDir::currentPath()+ "/settings.ini");
    QString settingsFile= (QDir::currentPath()+"/settig.ini");
    QSettings *settings =new QSettings(settingsFile,QSettings::IniFormat);
    if(!st.open(QIODevice::ReadOnly | QIODevice::Text))

    {

        settings->setValue("svrhost", "194.111.212.230");

        settings->setValue("erphost", "194.111.212.230");
        settings->setValue("erpuser", "sa");
        settings->setValue("erppassword", "STM2support");
        settings->setValue("erpdb","elina");

        settings->setValue("algohost", "194.111.212.230");
        settings->setValue("algouser", "sa");
        settings->setValue("algopassword", "STM2support");
        settings->setValue("algodb","elina_sqlserver");

        settings->setValue("exthost", "194.111.212.249");
        settings->setValue("extuser", "sa");
        settings->setValue("extpassword", "STM2support");
        settings->setValue("extdb","elina_prodiagrafes");

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
