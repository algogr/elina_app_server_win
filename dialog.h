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

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QtNetwork>
#include <QtSql>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QKeyEvent>


namespace Ui {
    class Dialog;
}

class Dialog : public QDialog {
    Q_OBJECT
public:
    Dialog(QWidget *parent = 0);
    ~Dialog();

protected:
    void changeEvent(QEvent *e);
    QSqlDatabase db;
    QSqlDatabase db1;
    QSqlDatabase db2;
    QSqlDatabase db3;
    QTcpServer server;
    //QDataStream *in;

    QList <QTcpSocket *> clientconnections;
    QMap <QString,QString> checkmap;

    quint16 nextblocksize;
    int m_sign;

    QString ftrid;
    void customer_names(QString like,QString lang,QTcpSocket *client);
    void item_codes(QString like,QTcpSocket *client);
    void fortoseis_progress(QTcpSocket *client);
    void profortoseis_progress(QTcpSocket *client);
    void profortoseis_closed(QTcpSocket *client);
    void fortosi_final(QTcpSocket *client,QDataStream *in);
    void fortosi_review(QString fsid,QTcpSocket *client);
    void profortosi_review(QString fsid,QTcpSocket *client);
    void fortosi_continue(QString fsid,QTcpSocket *client);
    void profortosi_continue(QString fsid,QTcpSocket *client);
    void fortosi_temporary(QDataStream *in,QTcpSocket *client);
    int  apografi_check(QString code_t);
    void compare(QTcpSocket *client);
    void check_finish(QTcpSocket *client);
    void ins_finish(QTcpSocket *client);
    void update_pr_log();

private:
    Ui::Dialog *ui;
    QSystemTrayIcon *trayicon;
    void appendlog(QString function,QString querystr);
    bool debug;
    void keyPressEvent(QKeyEvent *e);
public slots:
    void acceptConnection();
    void startRead();
    void clientDisconnected();
    void error();
    void quitapp();
    void about();
    void log();
    void debugon();
    void debugoff();
    void clearlog();
};

#endif // DIALOG_H
