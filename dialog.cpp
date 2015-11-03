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

#include "dialog.h"
#include "ui_dialog.h"
#include <QMenu>
#include <QMessageBox>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
    QString settingsFile = (QDir::currentPath()+ "/settings.ini");
    QSettings *settings =new QSettings(settingsFile,QSettings::IniFormat);
    QString host=settings->value("host").toString();
    QString dbuser=settings->value("dbuser").toString();
    QString dbpassword=settings->value("dbpassword").toString();
    QString apath=settings->value("apath").toString();
    QString version=settings->value("version").toString();
    ui->label->setText(ui->label->text()+version);
    ui->tableWidget->setColumnCount(4);
    db1 = QSqlDatabase::addDatabase("QODBC","elina_sqlserver");
    db2 = QSqlDatabase::addDatabase("QODBC","elina");
    db3 = QSqlDatabase::addDatabase("QODBC","elina_prodiagrafes");
    db1.setDatabaseName("elina_sqlserver");
    db1.setHostName("localhost");
    db1.setUserName(dbuser);
    db1.setPassword(dbpassword);
    db2.setDatabaseName("elina");
    db2.setHostName("localhost");
    db3.setDatabaseName("elina_prodiagrafes");
    db3.setHostName("localhost");
    db3.setUserName(dbuser);
    db3.setPassword(dbpassword);
    db1 = QSqlDatabase::database("elina_sqlserver");
    db2 = QSqlDatabase::database("elina");
    db3 = QSqlDatabase::database("elina_prodiagrafes");
    if (!db1.open())
    {
        qDebug() << "1.";
        qDebug()<<db1.lastError();
    }
    if (!db2.open())
    {
        qDebug() << "2.";
        qDebug()<<db2.lastError();
    }

    if (!db3.open())
    {
        qDebug() << "3.";
        qDebug()<<db3.lastError();
    }
    connect(&server, SIGNAL(newConnection()),
            this, SLOT(acceptConnection()));

    QHostAddress addr(host);

    //SCANNER
    server.listen(addr, 8889);




    nextblocksize=0;
    //in=0;
    m_sign=0;
    QSystemTrayIcon *trayicon=new QSystemTrayIcon();
    QString ipath=apath+"server-5.png";
    QIcon *icon=new QIcon(ipath);
    QMenu *traymenu=new QMenu();
    trayicon->setToolTip("Algo Application Server");

    trayicon->setContextMenu(traymenu);
    QAction *actionlog=traymenu->addAction(tr("Log"));
    QAction *actionabout=traymenu->addAction(tr("About"));
    traymenu->addSeparator();
    QAction *actionexit=traymenu->addAction(tr("Exit"));

    trayicon->setIcon(*icon);
    trayicon->show();
    connect(actionexit,SIGNAL(triggered()),this,SLOT(quitapp()));
    connect(actionabout,SIGNAL(triggered()),this,SLOT(about()));
    connect(actionlog,SIGNAL(triggered()),this,SLOT(log()));
    connect(ui->pushButton,SIGNAL(released()),this,SLOT(hide()));
    delete settings;
}

Dialog::~Dialog()
{
    server.close();
    delete ui;
}

void Dialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void Dialog::acceptConnection()
{

  QTcpSocket *client = server.nextPendingConnection();
  //QDataStream *in=new QDataStream (client);
  //in->setVersion(QDataStream::Qt_4_1);
  qDebug()<<"1.BUFFERSIZE:"<<client->readBufferSize();
  connect(client, SIGNAL(readyRead()),
                    this, SLOT(startRead()));
  connect(client, SIGNAL(error(QAbstractSocket::SocketError)),
              this, SLOT(error()));
  connect(client, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
  clientconnections.append(client);
}

void Dialog::clientDisconnected()
{
    QTcpSocket *client=qobject_cast<QTcpSocket *>(sender());
    if (!client)
            return;
    //delete(in);
    clientconnections.removeAll(client);
    client->deleteLater();
}




void Dialog::startRead()
{
        QTcpSocket *client=qobject_cast<QTcpSocket *>(sender());
        QDataStream *in=new QDataStream (client);
        in->setVersion(QDataStream::Qt_4_1);


        qDebug()<<"2.BYETS1:"<<client->bytesAvailable();
        //QDataStream in(client);
        //in.setVersion(QDataStream::Qt_4_1);
        //nextblocksize=0;
        //int m=0;
        int compare_mode=0;
        int check_mode=0;
        int ins_mode=0;
        int pc=0;
        //QString ftrid;
        qDebug()<<"3.DIAVASA KATI";
        forever
        {
        if (nextblocksize==0) {
                if (client->bytesAvailable()<sizeof(quint16))
            {
                    qDebug()<<"4.liga bytes available";
                                //return;
                        break;
                            }
                *in >> nextblocksize;
                qDebug() << "5.ARXH"<< nextblocksize;


                }

                if (nextblocksize==0xFFFF)
                {
                    qDebug()<<"6.VGIKA:";
                    if (compare_mode==1)
                    {
                        compare(client);
                        compare_mode=0;
                    }
                    if (check_mode==1)
                    {
                        QMapIterator <QString,QString> i(checkmap);

                        while (i.hasNext())
                        {
                            i.next();
                            QByteArray block;
                            QDataStream out(&block,QIODevice::WriteOnly);
                            out.setVersion(QDataStream::Qt_4_1);
                            QString type="CHKC";

                            out<<quint16(0)<<(QString)type<<(QString)i.key()<<(QString)i.value();
                            qDebug()<<"CHKECKING"<<i.key();
                        out.device()->seek(0);
                        out<<quint16(block.size()-sizeof(quint16));
                        //qDebug()<<"blocksize:"<<quint16(block.size()-sizeof(quint16));
                        client->write(block);
                        //i.next();
                    }
                        check_finish(client);
                        checkmap.clear();
                        check_mode=0;
                    }
                    if (ins_mode==1)
                    {
                        ins_finish(client);
                        ins_mode=0;
                    }
                    //client->close();
                        nextblocksize=0;
                        m_sign=0;
                        //break;
                        return;
                }

                if (client->bytesAvailable()<nextblocksize){
                    qDebug()<<"12.EXO LIGOTERA";

                    //return;
                    break;
                }
                QString	req_type;
                *in >> req_type;

                qDebug() <<"7."<<req_type;



                if (req_type=="RC"){
                    QString like,lang;

                    *in >> like >> lang;

                    customer_names(like,lang,client);
                }


                if (req_type=="RI"){
                    QString like;
                    *in >> like;
                    item_codes(like,client);
                }

                if (req_type=="APCH"){

                    QString code_t;
                    *in >> code_t;

                    apografi_check(code_t);
                }


        if (req_type=="RF")
                fortoseis_progress(client);

        if (req_type=="PRF")
                profortoseis_progress(client);

        if (req_type=="PRFC")
        {
            QString customer;
            *in >> customer;
            QSqlQuery query(db1);
            QDateTime ftrdate=QDateTime::currentDateTime();

            query.exec("SELECT CCODE,CUSTOMER,CAR1,CAR2,WEIGHT,ID FROM PROFORTOSI_HEADER WHERE ISCLOSED=1 AND CCODE='"+customer+"'");
            while (query.next())
            {
                    QByteArray block;

                    QDataStream out(&block,QIODevice::WriteOnly);
                    out.setVersion(QDataStream::Qt_4_1);
                    QString ccode,cname,car1,car2,weight,id;
                    ccode=query.value(0).toString();
                    cname=query.value(1).toString();
                    car1=query.value(2).toString();
                    car2=query.value(3).toString();
                    weight=query.value(4).toString();
                    id=query.value(5).toString();
                    //qDebug()<< cname << car1 << car2 << weight;
                    out<<quint16(0)<<(QString)ccode<<(QString)cname << (QString)car1 << (QString)car2 << (QString)weight<<id;
                    out.device()->seek(0);
                    out<<quint16(block.size()-sizeof(quint16));

                    client->write(block);
            }
            QByteArray block;
            QDataStream out(&block,QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_1);
            out<<quint16(0xFFFF);
            client->write(block);

        }




        if (req_type=="RE")
        {
                QString fsid;
                *in >> fsid;

                fortosi_review(fsid,client);
        }

        if (req_type=="RPF")
                 profortoseis_closed(client);


        if (req_type=="PRE")
        {
                QString fsid;
                *in >> fsid;

                profortosi_review(fsid,client);
        }

        if (req_type=="RS")
        {
                QString fsid;
                *in >> fsid;


                fortosi_continue(fsid,client);
        }

        if (req_type=="PRS")
        {
                QString fsid;
                *in >> fsid;

                profortosi_continue(fsid,client);
        }




        if (req_type=="FP")
        {
                ins_mode=1;
                QString	ccode,customer,car1,car2,code_t,weight,pfid;

                *in >> ccode >> customer>>car1>>car2>>code_t>>weight>>pfid;

                qDebug()<<"8.jim:"<<ccode<<customer<<car1<<car2<<code_t<<weight<<pfid;
                qDebug()<<"9.BYETS2:"<<client->bytesAvailable();
                QSqlQuery query(db1);
                QDateTime ftrdate=QDateTime::currentDateTime();

                if (m_sign==0)
                        {
                        query.exec("INSERT INTO fortosi_header (ftrdate,customer,car1,car2,isclosed,iskef,weight,pfid,ccode) values ('"
                                        +ftrdate.toString("MM-dd-yyyy")+"','"+customer+"','"+car1+"','"+car2+"',1,0,"+weight+","+pfid+",'"+ccode+"')");
                        query.exec("UPDATE profortosi_header set istransformed=1 where id="+pfid);
                        query.exec("SELECT max(id) from fortosi_header where ftrdate='"+ftrdate.toString("MM-dd-yyyy")+
                                        "' and customer='"+customer+"' and car1='"+car1+"' and car2='"+car2+"'");
                        query.last();

                        ftrid=query.value(0).toString();

                        m_sign=1;
                        }
                query.exec("INSERT INTO fortosi_details (code_t,ftrid) values ('"+code_t+"',"+ftrid+")");
                QSqlQuery query1(db2);
                query1.exec("UPDATE SERIAL SET IS_FTIED=1 where SNSERIALC='"+code_t+"'");
        }

        if (req_type=="COMP")
        {
            QString code_t,prfid;

            *in >> code_t >> prfid;

            QSqlQuery query(db1);

            query.exec("INSERT INTO tmp_compare (prfid,code_t) values("+prfid+",'"+code_t+"')");
            compare_mode=1;
        }

        if (req_type=="DELPRF")
        {
            QString prfid;

            *in >> prfid;

            QSqlQuery query(db1);

            query.exec("DELETE FROM profortosi_details WHERE ftrid="+prfid);
            query.exec("DELETE FROM profortosi_header WHERE id="+prfid);

        }

        if (req_type=="DELF")
        {
            QString prfid;

            *in >> prfid;

            QSqlQuery query(db1);

            query.exec("DELETE FROM fortosi_details WHERE ftrid="+prfid);
            query.exec("DELETE FROM fortosi_header WHERE id="+prfid);

        }

        if (req_type=="CHKC")
        {
            pc=pc+1;
            check_mode=1;
            QString code_t;
            *in >> code_t;
            QSqlQuery query(db2);
            query.exec("SELECT * from serial where snserialc='"+code_t+"'" );
            if (!query.next())
            {
                qDebug()<<"9.DEN YPARXEIS";

                //QByteArray block;
                //QString type="CHKC";
                //QDataStream out(&block,QIODevice::WriteOnly);
                //QVariant test=pc;
                checkmap.insert(code_t,"0");
                //out.setVersion(QDataStream::Qt_4_1);
                //out<<quint16(0)<<(QString)type<<(QString)code_t<<(QString)"0"<<(QString)test.toString();
                //qDebug()<<code_t<<pc;
                //out.device()->seek(0);
                //out<<quint16(block.size()-sizeof(quint16));
                //qDebug()<<"blocksize:"<<quint16(block.size()-sizeof(quint16));
                //client->write(block);
                //if (pc==20)
                //{
                 //client->reset();
                 //pc=0;
                 //QByteArray block1;

                 //QDataStream out1(&block1,QIODevice::WriteOnly);
                 //out1.setVersion(QDataStream::Qt_4_1);
                 //out1<<quint16(0xFFFF);
                 //client->write(block1);
             //}

            }
            query.exec("SELECT * from serial where snserialc='"+code_t+"' and ((is_ptied=1) or (is_ftied=1) or (snDate3 is not null)) " );
            if (query.next())
            {
                qDebug()<<"10.YPARXEIS ALLA...";

                //QByteArray block;
                //QString type="CHKC";
                //QDataStream out(&block,QIODevice::WriteOnly);
                //out.setVersion(QDataStream::Qt_4_1);
                //out<<quint16(0)<<(QString)type<<(QString)code_t<<(QString)"1";
                //out.device()->seek(0);
                //out<<quint16(block.size()-sizeof(quint16));
                //client->write(block);
                //QByteArray block1;

                //QDataStream out1(&block1,QIODevice::WriteOnly);
                //out1.setVersion(QDataStream::Qt_4_1);
                //out1<<quint16(0xFFFF);
                //client->write(block1);
                checkmap.insert(code_t,"1");
            }

            //QByteArray block1;

            //QDataStream out1(&block1,QIODevice::WriteOnly);
            //out1.setVersion(QDataStream::Qt_4_1);
            //out1<<quint16(0xFFFF);
            //client->write(block1);


        }

        if (req_type=="PFP")
        {
                QString	ccode,customer,car1,car2,code_t,weight;
                ins_mode=1;
                *in >> ccode >> customer>>car1>>car2>>code_t>>weight;

                QSqlQuery query(db1);
                QDateTime ftrdate=QDateTime::currentDateTime();

                if (m_sign==0)
                        {
                        query.exec("INSERT INTO profortosi_header (ftrdate,customer,car1,car2,isclosed,weight,ccode) values ('"
                                        +ftrdate.toString("MM-dd-yyyy")+"','"+customer+"','"+car1+"','"+car2+"',1,"+weight+",'"+ccode+"')");
                        query.exec("SELECT max(id) from profortosi_header where ftrdate='"+ftrdate.toString("MM-dd-yyyy")+
                                        "' and customer='"+customer+"' and car1='"+car1+"' and car2='"+car2+"'");
                        query.last();

                        ftrid=query.value(0).toString();
                        m_sign=1;

                        }
                query.exec("INSERT INTO profortosi_details (code_t,ftrid) values ('"+code_t+"',"+ftrid+")");
                QSqlQuery query1(db2);
                query1.exec("UPDATE SERIAL SET IS_PTIED=1 where SNSERIALC='"+code_t+"'");

        }

        if (req_type=="APIN")
        {
                QString	code_t,code_a,comments;

                *in >> code_t >> code_a;
                comments=code_a.mid(15,-1);
                code_a=code_a.left(15);
                QSqlQuery query(db1);
                qDebug() << "code_a:" << code_a;
                QDateTime adate=QDateTime::currentDateTime();
                int i=apografi_check(code_t);

                bool res;
                if(i==0 )
                res=query.exec("INSERT INTO apografi (code_t,scanner,adate,code_a,comments) values ('"+code_t+"',1,'"+adate.toString("MM-dd-yyyy HH:mm:ss")+"','"+code_a+"','"+comments+"')");
                else
                    res=query.exec("INSERT INTO apografi_duplicate (code_t,scanner,adate,code_a,comments) values ('"+code_t+"',1,'"+adate.toString("MM-dd-yyyy HH:mm:ss")+"','"+code_a+"','"+comments+"')");
                /*
                if (res==TRUE)
                {


                    QByteArray block;
                    QString type="APIN";
                    QDataStream out(&block,QIODevice::WriteOnly);
                    out.setVersion(QDataStream::Qt_4_1);
                    out<<quint16(0)<<(QString)type<<(QString)code_t;
                    out.device()->seek(0);
                    out<<quint16(block.size()-sizeof(quint16));
                    client->write(block);
                    QByteArray block1;

                    QDataStream out1(&block1,QIODevice::WriteOnly);
                    out1.setVersion(QDataStream::Qt_4_1);
                    out1<<quint16(0xFFFF);
                    client->write(block1);

                }

*/

        }

        if (req_type=="APIN_NEW")
        {
            QString settingsFile = (QDir::currentPath()+ "/settings.ini");
            QSettings *settings =new QSettings(settingsFile,QSettings::IniFormat);
            QString hfsserver=settings->value("hfsserver").toString();
            QFile file(hfsserver+"apografi.txt");
            if(!file.open(QIODevice::ReadOnly))
            {
                qDebug()<<"File apografi not found";
            }
            QTextStream in(&file);
            QString line=in.readLine();
            QSqlQuery query(db1);
            QDateTime adate=QDateTime::currentDateTime();

            while (!line.isNull())
            {
                QString code_t,code_a,comments;
                code_t=line.left(15);
                code_a=line.mid(15,15);
                comments=line.mid(30,-1);

                int i=apografi_check(code_t);

                bool res;
                if(i==0 )
                    res=query.exec("INSERT INTO apografi (code_t,adate,code_a,comments) values ('"+code_t+"','"+adate.toString("MM-dd-yyyy HH:mm:ss")+"','"+code_a+"','"+comments+"')");
                else
                    res=query.exec("INSERT INTO apografi_duplicate (code_t,adate,code_a,comments) values ('"+code_t+"','"+adate.toString("MM-dd-yyyy HH:mm:ss")+"','"+code_a+"','"+comments+"')");

                line=in.readLine();
            }
            QString test=hfsserver+"apografi_"+adate.toString("MMddyyyyHHmmss")+".txt";
            qDebug()<<test;
            file.copy(hfsserver+"apografi_"+adate.toString("MMddyyyyHHmmss")+".txt");
            delete settings;

        }




        if (req_type=="AKAT")
        {
                QString	code_t,code_a;

                *in >> code_t >> code_a;

                QSqlQuery query(db2);
                QSqlQuery query1(db1);
                QDateTime adate=QDateTime::currentDateTime();
                query.exec("select max(stfileid) from strn");
                QString stmaxid;
                query.next();
                QVariant id=query.value(0).toInt()+1;
                stmaxid=id.toString();
                QString serial=code_t;
                QString weight= serial.left(3);

                bool res;
                QString sFileid;
                query.exec("select sfileid from smast where scode='"+code_a+"'");
                query.next();
                sFileid=query.value(0).toString();
                //query.exec("select max(snfileid) from serial");
                //query.next();
                //QVariant id1=query.value(0).toInt()+1;
                //QString snmaxid=id1.toString();




                QDateTime pr_date=QDateTime::currentDateTime();


                res=query.exec("INSERT INTO STRN(stFileid,STDATE,sfileid,STTRANSKIND,STDOC,STLOCATION,STQUANT,stcomment,sttime) VALUES ("+stmaxid+",CURRENT_DATE,"+sFileid+",47,'KAT',1,"+weight+",'"+serial+"',cast (current_time as timestamp))");
                query.exec("UPDATE SERIAL SET SNDATE2=CURRENT_DATE where snserialc='"+serial+"' and sfileid="+sFileid);
                query.exec("UPDATE SERIAL SET SNINVOICE2='KAT/FI' where snserialc='"+serial+"' and sfileid="+sFileid);
                query.exec("UPDATE SERIAL SET SNTRKIND2=47 where snserialc='"+serial+"' and sfileid="+sFileid);
                query.exec("UPDATE SERIAL SET SNSPACE2=1 where snserialc='"+serial+"' and sfileid="+sFileid);
                query.exec("UPDATE SERIAL SET SNPERSCODE2='00.0000' where snserialc='"+serial+"' and sfileid="+sFileid);
                query.exec("UPDATE SERIAL SET SNDATE3=CURRENT_DATE where snserialc='"+serial+"' and sfileid="+sFileid);
                query.exec("UPDATE SERIAL SET SNINVOICE3='KAT/FI' where snserialc='"+serial+"' and sfileid="+sFileid);
                query.exec("UPDATE SERIAL SET SNTRKIND3=47 where snserialc='"+serial+"' and sfileid="+sFileid);
                query.exec("UPDATE SERIAL SET SNSPACE3=1 where snserialc='"+serial+"' and sfileid="+sFileid);
                query.exec("UPDATE SERIAL SET SNPERSCODE3='00.0000' where snserialc='"+serial+"' and sfileid="+sFileid);
                query1.exec("INSERT INTO analyser (code_t,adate,code_a) values ('"+code_t+"','"+pr_date.toString("MM-dd-yyyy")+"','"+code_a+"')");



                if (res==TRUE)
                {


                    QByteArray block;
                    QString type="APIN";
                    QDataStream out(&block,QIODevice::WriteOnly);
                    out.setVersion(QDataStream::Qt_4_1);
                    out<<quint16(0)<<(QString)type<<(QString)code_t;
                    out.device()->seek(0);
                    out<<quint16(block.size()-sizeof(quint16));
                    client->write(block);
                    QByteArray block1;

                    QDataStream out1(&block1,QIODevice::WriteOnly);
                    out1.setVersion(QDataStream::Qt_4_1);
                    out1<<quint16(0xFFFF);
                    client->write(block1);

                }



        }

        if (req_type=="RETURN_ROLL")
        {
                QString	code_t,code_a;

                *in >> code_t >> code_a;

                QSqlQuery query(db2);

                query.exec("select max(stfileid) from strn");
                QString stmaxid;
                query.next();
                QVariant id=query.value(0).toInt()+1;
                stmaxid=id.toString();
                QString serial=code_t;
                QString weight= serial.left(3);

                bool res;
                QString sFileid;
                query.exec("select sfileid from smast where scode='"+code_a+"'");
                query.next();
                sFileid=query.value(0).toString();
                //query.exec("select max(snfileid) from serial");
                //query.next();
                //QVariant id1=query.value(0).toInt()+1;
                //QString snmaxid=id1.toString();




                QDateTime pr_date=QDateTime::currentDateTime();


                //res=query.exec("INSERT INTO STRN(stFileid,STDATE,sfileid,STTRANSKIND,STDOC,STLOCATION,STQUANT,stcomment,sttime) VALUES ("+stmaxid+",CURRENT_DATE,"+sFileid+",47,'KAT',1,"+weight+",'"+serial+"',cast (current_time as timestamp))");
                query.exec("UPDATE SERIAL SET SNDATE1=CURRENT_DATE where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNINVOICE1='EPISTROFI' where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNTRKIND1=84 where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNSPACE1=1 where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNPERSCODE1='00.0000' where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNDATE3=CURRENT_DATE where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNINVOICE3='EPISTROFI' where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNTRKIND3=7 where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNSPACE3=1 where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNPERSCODE2=NULL where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNDATE2=NULL where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNINVOICE2=NULL where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNTRKIND2=NULL where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNSPACE2=NULL where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNPERSCODE2=NULL where snserialc='"+serial+"'");
                query.exec("UPDATE SERIAL SET SNPERSNAME2=NULL where snserialc='"+serial+"'");



                if (res==TRUE)
                {


                    QByteArray block;
                    QString type="APIN";
                    QDataStream out(&block,QIODevice::WriteOnly);
                    out.setVersion(QDataStream::Qt_4_1);
                    out<<quint16(0)<<(QString)type<<(QString)code_t;
                    out.device()->seek(0);
                    out<<quint16(block.size()-sizeof(quint16));
                    client->write(block);
                    QByteArray block1;

                    QDataStream out1(&block1,QIODevice::WriteOnly);
                    out1.setVersion(QDataStream::Qt_4_1);
                    out1<<quint16(0xFFFF);
                    client->write(block1);

                }



        }















        if (req_type=="KFCHECK")
        {
                QString	f_code;

                *in >> f_code;

                QSqlQuery query(db1);
                query.exec("select * from smast where scode='"+f_code+"')");
                QString reply;
                if (!query.next())
                    reply="0";
                else
                    reply="1";

                QByteArray block;
                QString type="KFCHECK";
                QDataStream out(&block,QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_1);
                out<<quint16(0)<<(QString)type<<(QString)reply<<(QString)f_code;
                out.device()->seek(0);
                out<<quint16(block.size()-sizeof(quint16));
                client->write(block);
                QByteArray block1;

                QDataStream out1(&block1,QIODevice::WriteOnly);
                out1.setVersion(QDataStream::Qt_4_1);
                out1<<quint16(0xFFFF);
                client->write(block1);


        }



        if (req_type=="KFINSERT")
        {
                QString	code_t,code,pid,vardia;

                *in >> code_t >> code >> pid >> vardia;

                QSqlQuery query(db2);
                query.exec("select * from strn where stcomment='"+code_t+"' and STTRANSKIND=83");
                QString reply;
                if (query.next())
                    return;



                query.exec("select max(stfileid) from strn");
                QString stmaxid;
                query.next();
                QVariant id=query.value(0).toInt()+1;
                stmaxid=id.toString();



                query.exec("select max(snfileid) from serial");

                query.next();
                QVariant id1=query.value(0).toInt()+1;
                QString snmaxid=id1.toString();

                QString serial=code_t;
                QString weight= serial.left(3);
                QString sFileid;
                QString quality=serial.mid(12,1);
                query.exec("select sfileid from smast where scode='"+code+"'");
                query.next();
                sFileid=query.value(0).toString();
                QDateTime pr_date=QDateTime::currentDateTime();
                bool res1,res2;
                QString st1,st2;
                qDebug()<<"TEST"<<code.left(1);
                st1="INSERT INTO STRN(stFileid,STDATE,sfileid,STTRANSKIND,STDOC,STLOCATION,STQUANT,STCOMMENT,stcomment2,sttime) VALUES ("+stmaxid+",CURRENT_DATE,"+sFileid+",83,'PARAGOGI',1,"+weight+",'"+serial+"','"+vardia+"',cast (current_time as timestamp))";
                st2="INSERT INTO SERIAL(snfileid,Sfileid,snserialc,SNDATE1,SNINVOICE1,SNTRKIND1,SNSPACE1) VALUES ("+snmaxid+","+sFileid+",'"+serial+"',CURRENT_DATE,'PARAGOGI',83,1)";
                if ((quality!="K") and (code.left(1)!="E"))
                {


                    //st=query.exec("INSERT INTO STRN(stFileid,STDATE,sfileid,STTRANSKIND,STDOC,STLOCATION,STQUANT,STCOMMENT) VALUES ("+stmaxid+",'"+pr_date.toString("yyyy-MM-dd")+"',"+sFileid+",66,'PARAGOGI',1,"+weight+",'"+serial+"')");
                    //sn=query.exec("INSERT INTO SERIAL(snfileidD,Sfileid,snscode,SNDATE1,SNINVOICE1,SNTRKIND1,SNSPACE1) VALUES ("+snmaxid+","+sFileid+",'"+serial+"','"+pr_date.toString("yyyy-MM-dd")+"','PARAGOGI',66,1)");
                    res1=query.exec(st1);
                    res2=query.exec(st2);

                }

                if ((quality!="K") and (code.left(1)=="E"))
                {


                    //st=query.exec("INSERT INTO STRN(stFileid,STDATE,sfileid,STTRANSKIND,STDOC,STLOCATION,STQUANT,STCOMMENT) VALUES ("+stmaxid+",'"+pr_date.toString("yyyy-MM-dd")+"',"+sFileid+",66,'PARAGOGI',1,"+weight+",'"+serial+"')");
                    //sn=query.exec("INSERT INTO SERIAL(snfileidD,Sfileid,snscode,SNDATE1,SNINVOICE1,SNTRKIND1,SNSPACE1) VALUES ("+snmaxid+","+sFileid+",'"+serial+"','"+pr_date.toString("yyyy-MM-dd")+"','PARAGOGI',66,1)");
                    //res1=query.exec(st1);
                    qDebug()<<st2;
                    res2=query.exec(st2);

                }



                if (res1==TRUE && res2==TRUE)
                {
                    QSqlQuery query1(db1);
                    query1.exec("UPDATE PRODUCTION SET ISKEF=1 WHERE ID="+pid);
                }
                else
                {
                    if (res1==FALSE)
                        query.exec("INSERT INTO production_failed (statement) values ('"+st1+"')");
                    if (res2==FALSE)
                        query.exec("INSERT INTO production_failed (statement) values ('"+st2+"')");

                }


            }




        if (req_type=="KFREWRAP")
        {
                QString	old_code,code_t,old_acode,pid,vardia,new_acode,new_aa;

                *in >> old_code >> code_t >> old_acode >> new_acode >> pid >> vardia >> new_aa;
                qDebug()<<"OLDCODE:"<<old_code<<"CODE_T"<<code_t<<"CODE:"<<old_acode<<"PID:"<<pid<<"VARDIA"<<vardia<<"NEW_AA"<<new_aa;
                QSqlQuery query(db2),query2(db1);
                query2.exec("insert into rewrap (old_code,new_code,r_date,old_acode,new_acode) values ('"\
                            +old_code+"','"+code_t+"',getdate(),'"+old_acode+"','"+new_acode+"')");

                query.exec("select * from strn where stcomment='"+code_t+"' and STTRANSKIND=83");
                QString reply;
                if (query.next())
                {
                    qDebug()<<"TO VRIKA GMT";
                    nextblocksize=0;
                    return;
                }



                query.exec("select max(stfileid) from strn");
                QString stmaxid,stmaxid1;
                query.next();

                QVariant id=query.value(0).toInt()+1;
                QVariant id2=query.value(0).toInt()+2;
                stmaxid=id.toString();
                stmaxid1=id2.toString();


                query.exec("select max(snfileid) from serial");

                query.next();
                QVariant id1=query.value(0).toInt()+1;
                QString snmaxid=id1.toString();
                QString serial_old=old_code;
                QString weight_old=serial_old.left(3);
                QString serial=code_t;
                QString weight= serial.left(3);
                QString sFileid;
                QString quality=serial.mid(12,1);
                query.exec("select sfileid from smast where scode='"+new_acode+"'");
                query.next();
                sFileid=query.value(0).toString();
                QDateTime pr_date=QDateTime::currentDateTime();

                qDebug()<<"sFileid"<<sFileid<<stmaxid<<snmaxid;

                if (quality!="K")
                {


                    //st=query.exec("INSERT INTO STRN(stFileid,STDATE,sfileid,STTRANSKIND,STDOC,STLOCATION,STQUANT,STCOMMENT) VALUES ("+stmaxid+",'"+pr_date.toString("yyyy-MM-dd")+"',"+sFileid+",66,'PARAGOGI',1,"+weight+",'"+serial+"')");
                    //sn=query.exec("INSERT INTO SERIAL(snfileidD,Sfileid,snscode,SNDATE1,SNINVOICE1,SNTRKIND1,SNSPACE1) VALUES ("+snmaxid+","+sFileid+",'"+serial+"','"+pr_date.toString("yyyy-MM-dd")+"','PARAGOGI',66,1)");
                    QString querystr="INSERT INTO STRN(stFileid,STDATE,sfileid,STTRANSKIND,STDOC,STLOCATION,STQUANT,STCOMMENT,stcomment2,sttime) VALUES ("+stmaxid+",CURRENT_DATE,"+sFileid+",83,'ANATYLIXI',1,"+weight+",'"+serial+"','"+vardia+"',cast (current_time as timestamp))";
                    query.exec(querystr);
                    qDebug()<<querystr;
                    querystr="INSERT INTO SERIAL(snfileid,Sfileid,snserialc,SNDATE1,SNINVOICE1,SNTRKIND1,SNSPACE1) VALUES ("+snmaxid+","+sFileid+",'"+serial+"',CURRENT_DATE,'ANATYLIXI',83,1)";
                    query.exec(querystr);


                }
                QSqlQuery query1(db1);
                query1.exec("UPDATE PRODUCTION SET ISKEF=1 WHERE ID="+pid);
                if (new_aa=="1")
                {
                    QString querystr="SELECT sfileid from smast where scode='"+old_acode+"'";
                    qDebug()<<querystr;
                    query.exec(querystr);
                    query.next();
                    sFileid=query.value(0).toString();

                querystr="INSERT INTO STRN(stFileid,STDATE,sfileid,STTRANSKIND,STDOC,STLOCATION,STQUANT,STCOMMENT,stcomment2,sttime) VALUES ("+stmaxid1+",CURRENT_DATE,"+sFileid+",88,'ANATYLIXI',1,"+weight_old+",'"+serial_old+"','"+vardia+"',cast (current_time as timestamp))";
                qDebug()<<querystr;
                query.exec(querystr);
                //query.exec("DELETE from strn where sttranskind=83 and stcomment='"+old_code+"'");
                query.exec("DELETE from serial where snserialc='"+old_code+"'");
            }


            }



        if (req_type=="PRREWRAP")
        {
                QString	old_code,code_1,code_2,code_3,code_4,code_5;

                *in >> old_code >> code_1 >> code_2 >> code_3 >> code_4 >> code_5;
                qDebug()<<old_code<<code_1<<code_2<<code_3<<code_4<<code_5;

                QSqlQuery query(db1);
                query.exec(
                        "select top 1 id,pr_date,vardia,f_code,middle,middle_2,middle_3 from production where code_t='"
                                    + old_code + "'");
                query.next();
                QString id = query.value(0).toString();
                QString pr_date = query.value(1).toString();
                QString vardia = query.value(2).toString();
                QString f_code = query.value(3).toString();
                QString middle = query.value(4).toString();
                QString middle_2 = query.value(5).toString();
                QString middle_3 = query.value(6).toString();
                QString pid1,pid2,pid3,pid4,pid5;


                query.exec("DELETE from production where code_t='"+old_code+"'");
                QString code_t = code_1;
                if (code_t != "")
                {
                    QString weight = code_t.left(3);
                    QString qual = code_t.mid(12, 1);
                    QString aa = code_t.mid(11, 1);
                    query.exec(
                                    "INSERT INTO PRODUCTION(weight,quality,middle,aa,pr_date,f_code,isKef,code_t,middle_2,middle_3,vardia) VALUES ("
                                            + weight + ",'" + qual + "','" + middle + "'," + aa
                                            + ",'" + pr_date + "','" + f_code + "',0,'"
                                            + code_t + "','" + middle_2 + "','" + middle_3
                                            + "','" + vardia + "')");
                            query.exec("select top 1 id from production where code_t='"+ code_t + "'");
                            query.next();
                            pid1 = query.value(0).toString();

                    }
                if(code_2!="")
                {
                    QString code_t = code_2;
                    QString weight = code_t.left(3);
                    QString qual = code_t.mid(12, 1);
                    QString aa = code_t.mid(11, 1);
                    query.exec(
                                    "INSERT INTO PRODUCTION(weight,quality,middle,aa,pr_date,f_code,isKef,code_t,middle_2,middle_3,vardia) VALUES ("
                                            + weight + ",'" + qual + "','" + middle + "'," + aa
                                            + ",'" + pr_date + "','" + f_code + "',0,'"
                                            + code_t + "','" + middle_2 + "','" + middle_3
                                            + "','" + vardia + "')");
                            query.exec("select top 1 id from production where code_t='"+ code_t + "'");
                            query.next();
                            pid2 = query.value(0).toString();

                }

                if(code_3!="")
                {
                    QString code_t = code_3;
                    QString weight = code_t.left(3);
                    QString qual = code_t.mid(12, 1);
                    QString aa = code_t.mid(11, 1);
                    query.exec(
                                    "INSERT INTO PRODUCTION(weight,quality,middle,aa,pr_date,f_code,isKef,code_t,middle_2,middle_3,vardia) VALUES ("
                                            + weight + ",'" + qual + "','" + middle + "'," + aa
                                            + ",'" + pr_date + "','" + f_code + "',0,'"
                                            + code_t + "','" + middle_2 + "','" + middle_3
                                            + "','" + vardia + "')");
                            query.exec("select top 1 id from production where code_t='"+ code_t + "'");
                            query.next();
                            pid3 = query.value(0).toString();

                }

                if(code_4!="")
                {
                    QString code_t = code_4;
                    QString weight = code_t.left(3);
                    QString qual = code_t.mid(12, 1);
                    QString aa = code_t.mid(11, 1);
                    query.exec(
                                    "INSERT INTO PRODUCTION(weight,quality,middle,aa,pr_date,f_code,isKef,code_t,middle_2,middle_3,vardia) VALUES ("
                                            + weight + ",'" + qual + "','" + middle + "'," + aa
                                            + ",'" + pr_date + "','" + f_code + "',0,'"
                                            + code_t + "','" + middle_2 + "','" + middle_3
                                            + "','" + vardia + "')");
                            query.exec("select top 1 id from production where code_t='"+ code_t + "'");
                            query.next();
                            pid4 = query.value(0).toString();

                }

                if(code_5!="")
                {
                    QString code_t = code_5;
                    QString weight = code_t.left(3);
                    QString qual = code_t.mid(12, 1);
                    QString aa = code_t.mid(11, 1);
                    query.exec(
                                    "INSERT INTO PRODUCTION(weight,quality,middle,aa,pr_date,f_code,isKef,code_t,middle_2,middle_3,vardia) VALUES ("
                                            + weight + ",'" + qual + "','" + middle + "'," + aa
                                            + ",'" + pr_date + "','" + f_code + "',0,'"
                                            + code_t + "','" + middle_2 + "','" + middle_3
                                            + "','" + vardia + "')");
                            query.exec("select top 1 id from production where code_t='"+ code_t + "'");
                            query.next();
                            pid5 = query.value(0).toString();

                }



                QByteArray block;
                QDataStream out(&block,QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_1);
                out<<quint16(0)<<(QString)f_code<<(QString)vardia<<(QString)pid1<<(QString)pid2<<(QString)pid3<<(QString)pid4<<(QString)pid5;

                out.device()->seek(0);
                out<<quint16(block.size()-sizeof(quint16));
                client->write(block);
                QByteArray block1;
                QDataStream out1(&block1,QIODevice::WriteOnly);
                out1.setVersion(QDataStream::Qt_4_1);
                out1<<quint16(0xFFFF);
                client->write(block1);





            }




        if (req_type=="KFDELETE")
        {
                QString	code_t,code;

                *in >> code_t >> code;
                int exist=0;
                QSqlQuery query1(db1);
                query1.exec("SELECT * from profortosi_details where code_t='"+code_t+"'");

                if (!query1.next())
                    exist=0;

                query1.exec("SELECT * from fortosi_details where code_t='"+code_t+"'");

                if (!query1.next())
                    exist=0;
                else
                    exist=1;
                if (exist==0)
                {
                QSqlQuery query(db2);
                query.exec("select stfileid from strn where stcomment='"+code_t+"'");

                if (query.next())
                {
                    QString id=query.value(0).toString();
                    query.exec("delete from strn where stfileid="+id);
                    query.exec("delete from serial where snserialc='"+code_t+"'");
                }
                QSqlQuery query1(db1);
                query1.exec("DELETE from production where code_t='"+code_t+"'");
            }
         }




        if (req_type=="CHLAB")
        {
                QString	oldcode,newcode;

                *in >> oldcode >> newcode;
                QDateTime chdate=QDateTime::currentDateTime();
                QSqlQuery query1(db1);
                query1.exec("INSERT INTO change_labels (oldcode,newcode,chdate) VALUES ('"+oldcode+"','"+newcode+"','"+chdate.toString("MM-dd-yyyy")+"')");
                query1.exec("select f_code,vardia from production where code_t='"+oldcode+"'");
                query1.next();
                QString vardia=query1.value(1).toString();
                QString code_a=query1.value(0).toString();
                QSqlQuery query(db2);
                query.exec("select max(stfileid) from strn");
                QString stmaxid;
                query.next();


                QVariant id=query.value(0).toInt()+1;
                stmaxid=id.toString();

                query.exec("select sfileid from smast where scode='"+code_a+"'");
                query.next();
                QString sFileid=query.value(0).toString();
                QString weight=oldcode.left(3);
                query.exec("INSERT INTO STRN(stFileid,STDATE,sfileid,STTRANSKIND,STDOC,STLOCATION,STQUANT,STCOMMENT,stcomment2,sttime) VALUES ("+stmaxid+",CURRENT_DATE,"+sFileid+",89,'ALLAGI',1,"+weight+",'"+oldcode+"','"+vardia+"',cast (current_time as timestamp))");
                query.exec("delete from serial where snserialc='"+oldcode+"'");


            }

        if (req_type=="READLAB")
        {
                QString	code;

                *in >> code;



                QSqlQuery query2(db2);
                QSqlQuery query3(db3);
                query2.exec("select sfileid from serial where snserialc='"+code+"'");
                query2.next();
                QString sid=query2.value(0).toString();
                query2.exec("select scode,sname from smast where sfileid="+sid);
                query2.next();
                QString scode=query2.value(0).toString();
                QString sname=query2.value(1).toString();
                query3.exec("SELECT xrisi_perigrafi+' '+onomasia,GRAMMARIA,EPIMIK,FULLA,MD,CD FROM PRODIAGRAFES where kodikos_p='"+scode+"'");
                query3.next();
                QString xrisi=query3.value(0).toString();
                QString grm=query3.value(1).toString();
                QString epim=query3.value(2).toString();
                QString fylla=query3.value(3).toString();
                QString md=query3.value(4).toString();
                QString cd=query3.value(5).toString();
                QByteArray block;
                QDataStream out(&block,QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_1);
                out<<quint16(0)<<(QString)scode<<(QString)sname<<(QString)xrisi<<grm<<epim<<fylla<<md<<cd;

                out.device()->seek(0);
                out<<quint16(block.size()-sizeof(quint16));
                client->write(block);
                QByteArray block1;
                QDataStream out1(&block1,QIODevice::WriteOnly);
                out1.setVersion(QDataStream::Qt_4_1);
                out1<<quint16(0xFFFF);
                client->write(block1);

            }







        if (req_type=="FT")
        {
                ins_mode=1;
                QString	ccode,customer,car1,car2,code_t,weight,pfid;

                                *in >> ccode >>customer>>car1>>car2>>code_t>>weight>>pfid;

                                QSqlQuery query(db1);
                                QDateTime ftrdate=QDateTime::currentDateTime();

                                if (m_sign==0)
                                        {
                                        query.exec("INSERT INTO fortosi_header (ftrdate,customer,car1,car2,isclosed,iskef,weight,pfid,ccode) values ('"
                                                        +ftrdate.toString("MM-dd-yyyy")+"','"+customer+"','"+car1+"','"+car2+"',0,0,"+weight+","+pfid+",'"+ccode+"')");
                                        query.exec("SELECT max(id) from fortosi_header where ftrdate='"+ftrdate.toString("MM-dd-yyyy")+
                                                        "' and customer='"+customer+"' and car1='"+car1+"' and car2='"+car2+"'");
                                        query.last();

                                        ftrid=query.value(0).toString();
                                        m_sign=1;
                                        }
                                query.exec("INSERT INTO fortosi_details (code_t,ftrid) values ('"+code_t+"',"+ftrid+")");
                                QSqlQuery query1(db2);
                                query1.exec("UPDATE SERIAL SET IS_FTIED=2 where SNSERIALC='"+code_t+"'");


        }

        if (req_type=="PFT")
        {
                QString	ccode,customer,car1,car2,code_t,weight;
                ins_mode=1;
                                *in >> ccode >> customer>>car1>>car2>>code_t>>weight;

                                QSqlQuery query(db1);
                                QDateTime ftrdate=QDateTime::currentDateTime();

                                if (m_sign==0)
                                        {
                                        query.exec("INSERT INTO profortosi_header (ftrdate,customer,car1,car2,isclosed,weight,ccode) values ('"
                                                        +ftrdate.toString("MM-dd-yyyy")+"','"+customer+"','"+car1+"','"+car2+"',0,"+weight+",'"+ccode+"')");
                                        query.exec("SELECT max(id) from profortosi_header where ftrdate='"+ftrdate.toString("MM-dd-yyyy")+
                                                        "' and customer='"+customer+"' and car1='"+car1+"' and car2='"+car2+"'");
                                        query.last();

                                        ftrid=query.value(0).toString();
                                        m_sign=1;
                                        }
                                query.exec("INSERT INTO profortosi_details (code_t,ftrid) values ('"+code_t+"',"+ftrid+")");
                                QSqlQuery query1(db2);
                                query1.exec("UPDATE SERIAL SET IS_PTIED=2 where SNSERIALC='"+code_t+"'");


        }
        if (req_type=="SF")
                        {
                                QString	customer,code_t,weight,fid;
                                ins_mode=1;
                                *in >> customer >> code_t >> weight >> fid;

                                QSqlQuery query(db1);
                                QDateTime ftrdate=QDateTime::currentDateTime();

                                if (m_sign==0)
                                        {
                                        query.exec("UPDATE fortosi_header SET weight="+weight+" WHERE id="+fid);
                                        query.exec("UPDATE fortosi_header SET isclosed=1 WHERE id="+fid);
                                        query.exec("DELETE FROM fortosi_details WHERE ftrid="+fid);
                                        m_sign=1;
                                        }
                                query.exec("INSERT INTO fortosi_details (code_t,ftrid) values ('"+code_t+"',"+fid+")");

                        }


        if (req_type=="PSF")
                        {
                                QString	customer,code_t,weight,fid;

                                *in >> customer >> code_t >> weight >> fid;
                                ins_mode=1;
                                QSqlQuery query(db1);
                                QDateTime ftrdate=QDateTime::currentDateTime();

                                if (m_sign==0)
                                        {
                                        query.exec("UPDATE profortosi_header SET weight="+weight+" WHERE id="+fid);
                                        query.exec("UPDATE profortosi_header SET isclosed=1 WHERE id="+fid);
                                        query.exec("DELETE FROM profortosi_details WHERE ftrid="+fid);
                                        m_sign=1;
                                        }
                                query.exec("INSERT INTO profortosi_details (code_t,ftrid) values ('"+code_t+"',"+fid+")");

                        }


                        if (req_type=="ST")
                                {
                                        QString	customer,code_t,weight,fid;
                                        ins_mode=1;
                                        *in >> customer >> code_t >> weight >> fid;
                                        qDebug()<<"M1:"<<m_sign;
                                        QSqlQuery query(db1);
                                        QDateTime ftrdate=QDateTime::currentDateTime();

                                        if (m_sign==0)
                                                {
                                                qDebug()<<"M2:"<<m_sign;
                                                query.exec("UPDATE fortosi_header SET weight="+weight+" WHERE id="+fid);
                                                query.exec("DELETE FROM fortosi_details WHERE ftrid="+fid);
                                                m_sign=1;
                                                }
                                        qDebug()<<"M3:"<<m_sign;
                                        query.exec("INSERT INTO fortosi_details (code_t,ftrid) values ('"+code_t+"',"+fid+")");

                                }
                        if (req_type=="PST")
                                {
                                        QString	customer,code_t,weight,fid;
                                        ins_mode=1;
                                        *in >> customer >> code_t >> weight >> fid;

                                        QSqlQuery query(db1);
                                        QDateTime ftrdate=QDateTime::currentDateTime();

                                        if (m_sign==0)
                                                {
                                                query.exec("UPDATE profortosi_header SET weight="+weight+" WHERE id="+fid);
                                                query.exec("DELETE FROM profortosi_details WHERE ftrid="+fid);
                                                m_sign=1;
                                                }
                                        query.exec("INSERT INTO profortosi_details (code_t,ftrid) values ('"+code_t+"',"+fid+")");

                                }



        //++m;
        nextblocksize=0;
        qDebug()<<"11.To nextblocksize einai:"<<nextblocksize;
        }







   //client->close();
}


int Dialog::apografi_check(QString code_t)

{

    QSqlQuery query(db1);
    query.exec("select * from apografi where code_t='"+code_t+"'");
    //QString reply;
    int i=0;
    if (!query.next())
        //reply="0";
        i=0;
    else
        //reply="1";
        i=1;
    return i;
    //qDebug()<<"reply:"<< reply;
    //QByteArray block;

    //QDataStream out(&block,QIODevice::WriteOnly);
    //out.setVersion(QDataStream::Qt_4_1);
    //out<<quint16(0)<<(QString)reply<<(QString)code_t;
    //out.device()->seek(0);
    //out<<quint16(block.size()-sizeof(quint16));
    //client->write(block);
    //QByteArray block1;

    //QDataStream out1(&block1,QIODevice::WriteOnly);
    //out1.setVersion(QDataStream::Qt_4_1);
    //out1<<quint16(0xFFFF);
    //client->write(block1);


}






void Dialog::customer_names(QString like,QString lang, QTcpSocket *client)
{

        //QSqlQuery query(db1);
        QSqlQuery query(db2);

        if (lang=="EL")
                {
                        like.replace(QString("C"),QString("Ø"));
                        like.replace(QString("D"),QString("Ä"));
                        like.replace(QString("F"),QString("Ö"));
                        like.replace(QString("G"),QString("Ã"));
                        like.replace(QString("J"),QString("Î"));
                        like.replace(QString("L"),QString("Ë"));
                        like.replace(QString("P"),QString("Ð"));
                        like.replace(QString("R"),QString("Ñ"));
                        like.replace(QString("S"),QString("Ó"));
                        like.replace(QString("U"),QString("È"));
                        like.replace(QString("V"),QString("Ù"));
                }

        query.exec("SELECT ccode,cname from cmast where cname like KC('"+like+"%') order by cname");

        //query.first();





        while (query.next())
        {
                QByteArray block;

                QDataStream out(&block,QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_1);
                QString ccode,cname;
                ccode=query.value(0).toString();
                cname=query.value(1).toString();

                out<<quint16(0)<<(QString)ccode<<(QString)cname;
                out.device()->seek(0);
                out<<quint16(block.size()-sizeof(quint16));

                client->write(block);
        }
        QByteArray block;
        QDataStream out(&block,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_1);
        out<<quint16(0xFFFF);
        client->write(block);
        return;
        //client->close();
}


void Dialog::item_codes(QString like,QTcpSocket *client)
{
        //QSqlQuery query(db1);
        QSqlQuery query(db2);
        query.exec("SELECT scode from smast where scode like KC('"+like+"%') order by scode");
        //query.first();





        while (query.next())
        {
                QByteArray block;

                QDataStream out(&block,QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_1);
                QString scode;
                scode=query.value(0).toString();

                out<<quint16(0)<<(QString)scode;
                out.device()->seek(0);
                out<<quint16(block.size()-sizeof(quint16));

                client->write(block);
        }
        QByteArray block;
        QDataStream out(&block,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_1);
        out<<quint16(0xFFFF);
        client->write(block);
        //client->close();
}


void Dialog::fortoseis_progress(QTcpSocket *client)
{
        QSqlQuery query(db1);

        query.exec("SELECT ccode,customer,car1,car2,weight,id from fortosi_header where isclosed=0 order by customer");





        while (query.next())
        {
                QByteArray block;

                QDataStream out(&block,QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_1);
                QString ccode,cname,car1,car2,weight,id;
                ccode=query.value(0).toString();
                cname=query.value(1).toString();
                car1=query.value(2).toString();
                car2=query.value(3).toString();
                weight=query.value(4).toString();
                id=query.value(5).toString();
                //qDebug()<< cname << car1 << car2 << weight;
                out<<quint16(0)<<(QString)ccode <<(QString)cname << (QString)car1 << (QString)car2 << (QString)weight<<id;
                out.device()->seek(0);
                out<<quint16(block.size()-sizeof(quint16));

                client->write(block);
        }
        QByteArray block;
        QDataStream out(&block,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_1);
        out<<quint16(0xFFFF);
        client->write(block);
        //client->close();
}

void Dialog::profortoseis_progress(QTcpSocket *client)
{
        QSqlQuery query(db1);

        query.exec("SELECT customer,car1,car2,weight,id from profortosi_header where isclosed=0 order by customer");





        while (query.next())
        {
                QByteArray block;

                QDataStream out(&block,QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_1);
                QString cname,car1,car2,weight,id;
                cname=query.value(0).toString();
                car1=query.value(1).toString();
                car2=query.value(2).toString();
                weight=query.value(3).toString();
                id=query.value(4).toString();
                //qDebug()<< cname << car1 << car2 << weight;
                out<<quint16(0)<<(QString)cname << (QString)car1 << (QString)car2 << (QString)weight<<id;
                out.device()->seek(0);
                out<<quint16(block.size()-sizeof(quint16));

                client->write(block);
        }
        QByteArray block;
        QDataStream out(&block,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_1);
        out<<quint16(0xFFFF);
        client->write(block);
        //client->close();
}




void Dialog::fortosi_review(QString fsid,QTcpSocket *client)
{
        QSqlQuery query(db1);

        query.exec("SELECT h.customer,h.car1,h.car2,h.weight,d.code_t from fortosi_header h,fortosi_details d where d.ftrid=h.id and h.id="+fsid+" order by d.id");





        while (query.next())
        {
                QByteArray block;

                QDataStream out(&block,QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_1);
                QString cname,car1,car2,weight,code_t;
                cname=query.value(0).toString();
                car1=query.value(1).toString();
                car2=query.value(2).toString();
                weight=query.value(3).toString();
                code_t=query.value(4).toString();
                //qDebug()<< cname << car1 << car2 << weight;
                out<<quint16(0)<<(QString)cname << (QString)car1 << (QString)car2 << (QString)weight<<code_t;
                out.device()->seek(0);
                out<<quint16(block.size()-sizeof(quint16));

                client->write(block);
        }
        QByteArray block1;
        QDataStream out1(&block1,QIODevice::WriteOnly);
        out1.setVersion(QDataStream::Qt_4_1);
        out1<<quint16(0xFFFF);
        client->write(block1);
        //client->close();
}

void Dialog::profortosi_review(QString fsid,QTcpSocket *client)
{
        QSqlQuery query(db1);

        query.exec("SELECT h.customer,h.car1,h.car2,h.weight,d.code_t from profortosi_header h,profortosi_details d where d.ftrid=h.id and h.id="+fsid+" order by d.id");





        while (query.next())
        {
                QByteArray block;

                QDataStream out(&block,QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_1);
                QString cname,car1,car2,weight,code_t;
                cname=query.value(0).toString();
                car1=query.value(1).toString();
                car2=query.value(2).toString();
                weight=query.value(3).toString();
                code_t=query.value(4).toString();
                //qDebug()<< cname << car1 << car2 << weight;
                out<<quint16(0)<<(QString)cname << (QString)car1 << (QString)car2 << (QString)weight<<code_t;
                out.device()->seek(0);
                out<<quint16(block.size()-sizeof(quint16));

                client->write(block);
        }
        QByteArray block;
        QDataStream out(&block,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_1);
        out<<quint16(0xFFFF);
        client->write(block);
        //client->close();
}


void Dialog::fortosi_continue(QString fsid,QTcpSocket *client)
{
        QSqlQuery query(db1);

        query.exec("SELECT h.customer,d.code_t,h.weight,h.pfid from fortosi_header h,fortosi_details d where d.ftrid=h.id and h.id="+fsid+" order by d.id");




        int pack=0;
        QVariant pack1;
        while (query.next())
        {
                QByteArray block;
                pack+=1;
                pack1=pack;
                QDataStream out(&block,QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_1);
                QString type,cname,code_t,weight,prfid;
                type="RS";
                cname=query.value(0).toString();
                code_t=query.value(1).toString();
                weight=query.value(2).toString();
                prfid=query.value(3).toString();

                out<<quint16(0)<<type<<(QString)cname <<code_t << weight<<prfid<< pack1.toString();
                qDebug()<<type<<(QString)cname <<code_t << weight<<prfid<< pack1.toString();
                out.device()->seek(0);
                out<<quint16(block.size()-sizeof(quint16));

                client->write(block);
        }
        QByteArray block;
        QDataStream out(&block,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_1);
        out<<quint16(0xFFFF);
        client->write(block);
        //client->close();
}

void Dialog::profortosi_continue(QString fsid,QTcpSocket *client)
{
        QSqlQuery query(db1);

        query.exec("SELECT h.customer,d.code_t,h.weight from profortosi_header h,profortosi_details d where d.ftrid=h.id and h.id="+fsid+" order by d.id");





        while (query.next())
        {
                QByteArray block;

                QDataStream out(&block,QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_1);
                QString type,cname,code_t,weight;
                type="PRS";
                cname=query.value(0).toString();
                code_t=query.value(1).toString();
                weight=query.value(2).toString();
                //qDebug()<< cname << car1 << car2 << weight;
                out<<quint16(0)<<type<<(QString)cname <<code_t << weight;
                out.device()->seek(0);
                out<<quint16(block.size()-sizeof(quint16));

                client->write(block);
        }
        QByteArray block;
        QDataStream out(&block,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_1);
        out<<quint16(0xFFFF);
        client->write(block);
        //client->close();
}

void Dialog::profortoseis_closed(QTcpSocket *client)
{
        QSqlQuery query(db1);

        query.exec("SELECT ftrdate,id,customer,car1,car2,weight,ccode FROM profortosi_header where isclosed=1 and istransformed=0 order by ftrdate");





        while (query.next())
        {
                QByteArray block;

                QDataStream out(&block,QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_1);
                QString ftrdate,id,cname,car1,car2,weight,ccode;
                ftrdate=query.value(0).toString();
                id=query.value(1).toString();
                cname=query.value(2).toString();
                car1=query.value(3).toString();
                car2=query.value(4).toString();
                weight=query.value(5).toString();
                ccode=query.value(6).toString();
                //qDebug()<< cname << car1 << car2 << weight;
                out<<quint16(0)<<ftrdate<<id <<cname<<car1<<car2<<weight<<ccode;
                out.device()->seek(0);
                out<<quint16(block.size()-sizeof(quint16));

                client->write(block);
        }
        QByteArray block;
        QDataStream out(&block,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_1);
        out<<quint16(0xFFFF);
        client->write(block);

        //client->close();
}





void Dialog::fortosi_final(QTcpSocket *client,QDataStream *in)
{
                //QDataStream in(client);
                //in.setVersion(QDataStream::Qt_4_1);
                 int r=0;

                 forever
                        {
                                if (nextblocksize==0) {

                                        if (client->bytesAvailable()<sizeof(quint16))
                                                break;
                                *in >> nextblocksize;


                                }

                                if (nextblocksize==0xFFFF)
                                {
                                        //client->close();
                                        break;
                                }

                                if (client->bytesAvailable()<nextblocksize)
                                        break;



                                ++r;

                                //qDebug() << req_type;
                                nextblocksize=0;
                        }
                 //client->close();

}



void Dialog::fortosi_temporary(QDataStream *in,QTcpSocket *client)
{
        //QDataStream in1(client);
                        //in1.setVersion(QDataStream::Qt_4_1);
                         int r=0;

                         forever
                                {
                                        if (nextblocksize==0) {

                                                if (client->bytesAvailable()<sizeof(quint16))
                                                        break;
                                        *in >> nextblocksize;


                                        }

                                        if (nextblocksize==0xFFFF)
                                        {
                                                //client->close();
                                                break;
                                        }

                                        if (client->bytesAvailable()<nextblocksize)
                                                break;
                                        QString	customer,car1,car2,code_t;
                                        *in >> customer>>car1>>car2>>code_t;

                                        //qDebug()<<"CN"<<customer<<car1<<car2<<code_t;



                                        ++r;

                                        //qDebug() << req_type;
                                        nextblocksize=0;
                                }


                         //client->close();
}


void Dialog::compare(QTcpSocket *client)
{
    QSqlQuery query(db1);
    query.exec("SELECT t.code_t from tmp_compare t where not exists (select * from profortosi_details pf where pf.ftrid=t.prfid and pf.code_t=t.code_t) ");
    QSqlQuery query1(db1);
    query1.exec("select pf.code_t from profortosi_details pf where ftrid=(select distinct prfid from tmp_compare) and not exists (select * from tmp_compare t where t.code_t=pf.code_t)");
    while(query.next())
    {
        QByteArray block;
        QString type="COMP";
        QString code_t=query.value(0).toString();
        QDataStream out(&block,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_1);
        out<<quint16(0)<<(QString)type<<(QString)code_t<<(QString)"0";

        out.device()->seek(0);
        out<<quint16(block.size()-sizeof(quint16));
        client->write(block);
    }

    while(query1.next())
    {
        QByteArray block;
        QString type="COMP";
        QString code_t=query1.value(0).toString();
        QDataStream out(&block,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_1);
        out<<quint16(0)<<(QString)type<<(QString)code_t<<(QString)"1";

        out.device()->seek(0);
        out<<quint16(block.size()-sizeof(quint16));
        client->write(block);
    }

        QByteArray block1;

        QDataStream out1(&block1,QIODevice::WriteOnly);
        out1.setVersion(QDataStream::Qt_4_1);
        out1<<quint16(0xFFFF);
        client->write(block1);
        qDebug()<<"12.ESTEILA COMP";
        query.exec("DELETE from tmp_compare");

    }



void Dialog::check_finish(QTcpSocket *client)
{
    QByteArray block;
    qDebug()<<"ESTEILA CHKC";
    QString type="CHKC";
    QDataStream out(&block,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_1);
    out<<quint16(0)<<(QString)type<<(QString)"99999"<<(QString)"0";
    out.device()->seek(0);
    out<<quint16(block.size()-sizeof(quint16));
    client->write(block);
    QByteArray block1;

    QDataStream out1(&block1,QIODevice::WriteOnly);
    out1.setVersion(QDataStream::Qt_4_1);
    out1<<quint16(0xFFFF);
    client->write(block1);

}

void Dialog::ins_finish(QTcpSocket *client)
{
    QByteArray block;
    QString type="IFI";
    qDebug()<<"14.ESTEILA IFI";
    QDataStream out(&block,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_1);
    out<<quint16(0)<<(QString)type;
    out.device()->seek(0);
    out<<quint16(block.size()-sizeof(quint16));
    client->write(block);
    QByteArray block1;

    QDataStream out1(&block1,QIODevice::WriteOnly);
    out1.setVersion(QDataStream::Qt_4_1);
    out1<<quint16(0xFFFF);
    client->write(block1);
    m_sign=0;
}

void Dialog::error()
{
    QTcpSocket *client=qobject_cast<QTcpSocket *>(sender());
    qDebug()<<"N\etworkError"<<client->errorString();

}

void Dialog::update_pr_log(){
 //QSqlQuery query(*db1);
 //query.exec("SELECT")

}
void Dialog::quitapp()
{
    qDebug()<<"LALA";
    exit(0);
}

void Dialog::about()
{
    ui->pushOff->setVisible(FALSE);
    ui->pushOn->setVisible(FALSE);
    ui->tableWidget->setVisible(FALSE);
    ui->label->setVisible(TRUE);
    this->show();
    /*
    QMessageBox mb;
    mb.setParent(this->parent());
    mb.setStandardButtons(QMessageBox::Ok);
    mb.setText("Elina application server ");

    mb.setWindowTitle("Elina Application server");
    mb.exec();
    */
}

void Dialog::log()
{
    ui->pushOff->setVisible(TRUE);
    ui->pushOn->setVisible(TRUE);
    ui->tableWidget->setVisible(TRUE);
    ui->label->setVisible(FALSE);
    this->show();

}

void Dialog::appendlog(QString function,QString querystr)
{
    QDateTime *timestamp=new QDateTime(QDateTime::currentDateTime());
    QTableWidgetItem *d=new QTableWidgetItem;
    d->setText(timestamp->date().toString());

    QTableWidgetItem *t=new QTableWidgetItem;
    t->setText(timestamp->time().toString());

    QTableWidgetItem *f=new QTableWidgetItem;
    f->setText(function);

    QTableWidgetItem *q=new QTableWidgetItem;
    q->setText(querystr);

    int r=ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount(r+1);
    ui->tableWidget->setItem(r,0,d);
    ui->tableWidget->setItem(r,1,t);
    ui->tableWidget->setItem(r,2,f);
    ui->tableWidget->setItem(r,3,q);

}
