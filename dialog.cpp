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
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    this->setFocusPolicy(Qt::NoFocus);
    this->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
    debug=false;
    QString settingsFile = (QDir::currentPath()+ "/settings.ini");
    QSettings *settings =new QSettings(settingsFile,QSettings::IniFormat);
    QString version=settings->value("version").toString();
    QString apath=settings->value("apath").toString();

    ui->label->setText(ui->label->text()+version);

    QString svrhost=settings->value("svrhost").toString();

    QString algodb=settings->value("algodb").toString();
    QString algohost=settings->value("algohost").toString();
    QString algouser=settings->value("algouser").toString();
    QString algopassword=settings->value("algopassword").toString();

    QString erpdb=settings->value("erpdb").toString();
    QString erphost=settings->value("erphost").toString();
    QString erpuser=settings->value("erpuser").toString();
    QString erppassword=settings->value("erppassword").toString();

    QString extdb=settings->value("extdb").toString();
    QString exthost=settings->value("exthost").toString();
    QString extuser=settings->value("extuser").toString();
    QString extpassword=settings->value("extpassword").toString();

    ui->tableWidget->setColumnCount(4);
    db1 = QSqlDatabase::addDatabase("QODBC",algodb);
    db2 = QSqlDatabase::addDatabase("QODBC",erpdb);
    db3 = QSqlDatabase::addDatabase("QODBC",extdb);

    db1.setDatabaseName(algodb);
    db1.setHostName(algohost);
    db1.setUserName(algouser);
    db1.setPassword(algopassword);

    db2.setDatabaseName(erpdb);
    db2.setHostName(erphost);
    db2.setUserName(erpuser);
    db2.setPassword(erppassword);

    db3.setDatabaseName(extdb);
    db3.setHostName(exthost);
    db3.setUserName(extuser);
    db3.setPassword(extpassword);

    db1 = QSqlDatabase::database(algodb);
    db2 = QSqlDatabase::database(erpdb);
    db3 = QSqlDatabase::database(extdb);
    /*Check available drivers
    QString driverlist=QString("");
    QStringList drvList=QSqlDatabase::drivers();
    for (int i=0; i<drvList.count(); i++)
    {
        driverlist.append(drvList[i]);
    }
    QMessageBox::information(0,QString("DRIVERS:"),driverlist);
    */

    if (!db1.open())
    {

        QMessageBox::critical(0,"Error on Algo",db1.lastError().text());
        exit (0);
    }
    if (!db2.open())
    {
        QMessageBox::critical(0,"Error on ERP",db2.lastError().text());
        exit (0);
    }

    if (!db3.open())
    {
        QMessageBox::critical(0,"Error on Prodiagrafes",db3.lastError().text());
        exit (0);
    }
    connect(&server, SIGNAL(newConnection()),
            this, SLOT(acceptConnection()));

    QHostAddress addr(svrhost);

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
    connect(ui->pushOn,SIGNAL(released()),this,SLOT(debugon()));
    connect(ui->pushOff,SIGNAL(released()),this,SLOT(debugoff()));
    connect(ui->pushClear,SIGNAL(released()),this,SLOT(clearlog()));
    ui->tableWidget->setColumnWidth(3,550);
    ui->tableWidget->setHorizontalHeaderLabels((QStringList()<<"Date"<<"Time"<<"function"<<"Query"));
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
    QString querystr;

    qDebug()<<"2.BYETS1:"<<client->bytesAvailable();
    //QDataStream in(client);
    //in.setVersion(QDataStream::Qt_4_1);
    //nextblocksize=0;
    //int m=0;

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

            break;
        }
        QString	req_type;
        *in >> req_type;




////ERP
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








        if (req_type=="RE")
        {
            QString fsid;
            *in >> fsid;

            fortosi_review(fsid,client);
        }



        if (req_type=="RS")
        {
            QString fsid;
            *in >> fsid;


            fortosi_continue(fsid,client);
        }






        if (req_type=="FP")
        {
            ins_mode=1;
            QString	ccode,customer,car1,car2,code_t,weight;

            *in >> ccode >> customer>>car1>>car2>>code_t>>weight;

            qDebug()<<"8.jim:"<<ccode<<customer<<car1<<car2<<code_t<<weight;
            qDebug()<<"9.BYETS2:"<<client->bytesAvailable();
            QSqlQuery query(db1);
            QDateTime ftrdate=QDateTime::currentDateTime();

            if (m_sign==0)
            {
                querystr="INSERT INTO fortosi_header (ftrdate,customer,car1,car2,isclosed,iserp,weight,ccode) values ('"
                        +ftrdate.toString("MM-dd-yyyy HH:mm:ss")+"','"+customer+"','"+car1+"','"+car2+"',1,0,"+weight+",'"+ccode+"')";
                query.exec(querystr);
                if(debug)
                    appendlog("FP",querystr);
                querystr="SELECT max(id) from fortosi_header where dateadd(dd,0,datediff(dd,0,ftrdate))='"+ftrdate.toString("MM-dd-yyyy")+
                        "' and customer='"+customer+"' and car1='"+car1+"' and car2='"+car2+"'";
                query.exec(querystr);
                if(debug)
                    appendlog("FP",querystr);

                query.last();

                ftrid=query.value(0).toString();

                m_sign=1;
            }
            querystr="INSERT INTO fortosi_details (code_t,ftrid,weight) values ('"+code_t+"',"+ftrid+","+code_t.left(3)+")";
            query.exec(querystr);
            if(debug)
                appendlog("FP",querystr);


        }



        if (req_type=="DELF")
        {
            QString prfid;

            *in >> prfid;

            QSqlQuery query(db1);
            querystr="DELETE FROM fortosi_details WHERE ftrid="+prfid;
            query.exec(querystr);
            if(debug)
                appendlog("DELF",querystr);
            querystr="DELETE FROM fortosi_header WHERE id="+prfid;
            query.exec(querystr);
            if(debug)
                appendlog("DELF",querystr);


        }

        if (req_type=="CHKC")
        {
            pc=pc+1;
            check_mode=1;
            QString code_t;
            *in >> code_t;
            QSqlQuery query(db2);
            querystr="SELECT * from serial where snserialc='"+code_t+"'" ;
            query.exec(querystr);
            if(debug)
                appendlog("CHKC",querystr);

            if (!query.next())
                checkmap.insert(code_t,"0");


            querystr="SELECT * from serial where snserialc='"+code_t+"' and ((is_ptied=1) or (is_ftied=1) or (snDate3 is not null)) " ;
            query.exec(querystr);
            if(debug)
                appendlog("CHKC",querystr);

            if (query.next())
                checkmap.insert(code_t,"1");

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
                {
                    querystr="INSERT INTO apografi (code_t,adate,code_a,comments) values ('"+code_t+"','"+adate.toString("MM-dd-yyyy HH:mm:ss")+"','"+code_a+"','"+comments+"')";
                    res=query.exec(querystr);
                    if(debug)
                        appendlog("APIN_NEW",querystr);

                }
                else
                {
                    querystr="INSERT INTO apografi_duplicate (code_t,adate,code_a,comments) values ('"+code_t+"','"+adate.toString("MM-dd-yyyy HH:mm:ss")+"','"+code_a+"','"+comments+"')";
                    query.exec(querystr);
                    if(debug)
                        appendlog("APIN_NEW",querystr);

                }
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
            int islast;

            *in >> code_t >> code_a >> islast;
            QSqlQuery query(db1);
            querystr="INSERT INTO z_various (code_t,code_a,isreturn,isdestroyed,iserp) VALUES ('"+code_t+"','"+code_a+"',0,1,0)";
            query.exec(querystr);
            if(debug)
                appendlog("AKAT",querystr);



            if (islast==1)
                sendIFI(client);


        }











        if (req_type=="KFCHECK")
        {
            QString	f_code;

            *in >> f_code;

            QSqlQuery query(db1);
            querystr="select * from smast where scode='"+f_code+"')";
            query.exec(querystr);
            if(debug)
                appendlog("KFCHECK",querystr);

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




        if (req_type=="PRREWRAP")
        {
            QString	old_code_t,old_code_A,code_t,code_A;


            *in >> old_code_t >> old_code_A >> code_t >> code_A ;




            QSqlQuery query(db1);
            querystr="select top 1 pr_date from z_production where code_t='"
                    + old_code_t + "'";
            query.exec(querystr);
            if(debug)
                appendlog("PRREWRAP",querystr);
            query.next();

            QString pr_date = query.value(0).toString();

            querystr="INSERT INTO z_rewrap (oldcode_a,oldcode_t,newcode_a,newcode_t,rewrap_date,pr_date_oldcode,iserp)"
                     " VALUES ('"+old_code_A+"','"+old_code_t+"','"+code_A+"','"+code_t+"','"+\
                    QDateTime::currentDateTime().toString(Qt::ISODate)+"','"+pr_date+"',0)";
                    query.exec(querystr);

                    if(debug)
                        appendlog("PRREWRAP",querystr);
            sendIFI(client);

        }




        if (req_type=="CHLAB")
        {
            QString	oldcode,newcode;

            *in >> oldcode >> newcode;
            QDateTime chdate=QDateTime::currentDateTime();
            QSqlQuery query1(db1);
            querystr="INSERT INTO z_changelabel (oldcode_t,newcode_t,changedate) VALUES ('"+oldcode+"','"+newcode+"','"+chdate.toString("MM-dd-yyyy HH:mm:ss")+"')";
            query1.exec(querystr);
            if(debug)
                appendlog("CHLAB",querystr);


        }


        if (req_type=="RETURN_ROLL")
                {
                    QString	code_t,code_a;
                    int islast;

                    *in >> code_t >> code_a >> islast;
                    QSqlQuery query(db1);
                    querystr="INSERT INTO z_various (code_t,code_a,isreturn,isdestroyed,iserp) VALUES ('"+code_t+"','"+code_a+"',1,0,0)";
                    query.exec(querystr);
                    if(debug)
                        appendlog("RETURN_ROLL",querystr);



                    if (islast==1)
                        sendIFI(client);



                }


        if (req_type=="READLAB")
        {
            QString	code;

            *in >> code;



            QSqlQuery query2(db2);
            QSqlQuery query3(db3);
            querystr="select sfileid from serial where snserialc='"+code+"'";
            query2.exec(querystr);
            if(debug)
                appendlog("READLAB",querystr);
            query2.next();
            QString sid=query2.value(0).toString();
            querystr="select scode,sname from smast where sfileid="+sid;
            query2.exec(querystr);
            if(debug)
                appendlog("READLAB",querystr);

            query2.next();
            QString scode=query2.value(0).toString();
            QString sname=query2.value(1).toString();
            querystr="SELECT xrisi_perigrafi+' '+onomasia,GRAMMARIA,EPIMIK,FULLA,MD,CD FROM PRODIAGRAFES where kodikos_p='"+scode+"'";
            query3.exec(querystr);
            if(debug)
                appendlog("READLAB",querystr);
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
            QString	ccode,customer,car1,car2,code_t,weight;

            *in >> ccode >>customer>>car1>>car2>>code_t>>weight;

            QSqlQuery query(db1);
            QDateTime ftrdate=QDateTime::currentDateTime();

            if (m_sign==0)
            {
                querystr="INSERT INTO fortosi_header (ftrdate,customer,car1,car2,isclosed,iserp,weight,ccode) values ('"
                        +ftrdate.toString("MM-dd-yyyy HH:mm:ss")+"','"+customer+"','"+car1+"','"+car2+"',0,0,"+weight+",'"+ccode+"')";
                query.exec(querystr);
                if(debug)
                    appendlog("FT",querystr);
                querystr="SELECT max(id) from fortosi_header where dateadd(dd,0,datediff(dd,0,ftrdate))='"+ftrdate.toString("MM-dd-yyyy ")+
                        "' and customer='"+customer+"' and car1='"+car1+"' and car2='"+car2+"'";
                query.exec(querystr);
                if(debug)
                    appendlog("FT",querystr);
                query.last();

                ftrid=query.value(0).toString();
                m_sign=1;
            }

            querystr="INSERT INTO fortosi_details (code_t,ftrid,weight) values ('"+code_t+"',"+ftrid+","+code_t.left(3)+")";
            query.exec(querystr);
            if(debug)
                appendlog("FT",querystr);




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
                querystr="UPDATE fortosi_header SET weight="+weight+" WHERE id="+fid;
                query.exec(querystr);
                if(debug)
                    appendlog("SF",querystr);
                querystr="UPDATE fortosi_header SET isclosed=1 WHERE id="+fid;
                query.exec(querystr);
                if(debug)
                    appendlog("SF",querystr);
                querystr="UPDATE fortosi_header SET iserp=0 WHERE id="+fid;
                query.exec(querystr);
                if(debug)
                    appendlog("SF",querystr);

                querystr="DELETE FROM fortosi_details WHERE ftrid="+fid;
                query.exec(querystr);
                if(debug)
                    appendlog("SF",querystr);

                m_sign=1;
            }
            querystr="INSERT INTO fortosi_details (code_t,ftrid,weight) values ('"+code_t+"',"+fid+","+code_t.left(3)+")";
            query.exec(querystr);
            if(debug)
                appendlog("SF",querystr);

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
                querystr="UPDATE fortosi_header SET weight="+weight+" WHERE id="+fid;
                query.exec(querystr);
                if(debug)
                    appendlog("ST",querystr);
                querystr="UPDATE fortosi_header SET iserp=0 WHERE id="+fid;
                query.exec(querystr);
                if(debug)
                    appendlog("ST",querystr);
                querystr="DELETE FROM fortosi_details WHERE ftrid="+fid;
                query.exec(querystr);
                if(debug)
                    appendlog("ST",querystr);

                m_sign=1;
            }
            qDebug()<<"M3:"<<m_sign;
            querystr="INSERT INTO fortosi_details (code_t,ftrid,weight) values ('"+code_t+"',"+fid+","+code_t.left(3)+")";
            query.exec(querystr);
            if(debug)
                appendlog("ST",querystr);

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
    QString querystr="select * from apografi where code_t='"+code_t+"'";
    query.exec(querystr);
    if(debug)
        appendlog("apografi_check()",querystr);
    //QString reply;
    int i=0;
    if (!query.next())
        //reply="0";
        i=0;
    else
        //reply="1";
        i=1;
    return i;


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
    QString querystr="SELECT code,name from ESFITradeAccount where type=0 and name like ('"+like+"%') order by name";
    query.exec(querystr);
    if(debug)
        appendlog("customer_names()",querystr);


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
    QString querystr="SELECT scode from smast where scode like KC('"+like+"%') order by scode";
    query.exec(querystr);
    if(debug)
        appendlog("item_codes()",querystr);
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
    QString querystr="SELECT ccode,customer,car1,car2,weight,id from fortosi_header where isclosed=0 order by customer";
    query.exec(querystr);
    if(debug)
        appendlog("fortoseis_progress()",querystr);





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





void Dialog::fortosi_review(QString fsid,QTcpSocket *client)
{
    QSqlQuery query(db1);
    QString querystr="SELECT h.customer,h.car1,h.car2,h.weight,d.code_t from fortosi_header h,fortosi_details d where d.ftrid=h.id and h.id="+fsid+" order by d.id";
    query.exec(querystr);
    if(debug)
        appendlog("fortosi_review()",querystr);





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
    QString querystr="SELECT h.customer,h.car1,h.car2,h.weight,d.code_t from profortosi_header h,profortosi_details d where d.ftrid=h.id and h.id="+fsid+" order by d.id";
    query.exec(querystr);
    if(debug)
        appendlog("profortosi_review()",querystr);





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
    QString querystr="SELECT h.customer,d.code_t,h.weight,h.car1,h.car2 from fortosi_header h,fortosi_details d where d.ftrid=h.id and h.id="+fsid+" order by d.id";
    query.exec(querystr);
    if(debug)
        appendlog("fortosi_continue",querystr);




    int pack=0;
    QVariant pack1;
    while (query.next())
    {
        QByteArray block;
        pack+=1;
        pack1=pack;
        QDataStream out(&block,QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_1);
        QString type,cname,code_t,weight;
        type="RS";
        cname=query.value(0).toString();
        code_t=query.value(1).toString();
        weight=query.value(2).toString();

        out<<quint16(0)<<type<<(QString)cname <<code_t << weight<<pack1.toString();

        out.device()->seek(0);
        out<<quint16(block.size()-sizeof(quint16));

        client->write(block);
        qDebug()<<"times";
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
    QString querystr="SELECT h.customer,d.code_t,h.weight from profortosi_header h,profortosi_details d where d.ftrid=h.id and h.id="+fsid+" order by d.id";
    query.exec(querystr);
    if(debug)
        appendlog("profortosi_continue",querystr);





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





void Dialog::check_finish(QTcpSocket *client)
{
    QByteArray block;

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
    sendIFI(client);
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

    exit(0);
}

void Dialog::about()
{
    ui->pushOff->setVisible(false);
    ui->pushOn->setVisible(false);
    ui->pushClear->setVisible(false);
    ui->tableWidget->setVisible(false);
    ui->label->setVisible(true);
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
    ui->pushClear->setVisible(true);
    ui->pushOff->setVisible(true);

    ui->pushOn->setVisible(true);
    if (debug)
        ui->pushOn->setEnabled(false);
    else
        ui->pushOff->setEnabled(false);
    ui->tableWidget->setVisible(true);
    ui->label->setVisible(false);
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

void Dialog::debugon()
{
    debug=true;
    ui->pushOn->setEnabled(false);
    ui->pushOff->setEnabled(true);
}

void Dialog::debugoff()
{
    debug=false;
    ui->pushOn->setEnabled(true);
    ui->pushOff->setEnabled(false);

}

void Dialog::clearlog()
{
    while(ui->tableWidget->rowCount()>0)
    {
        ui->tableWidget->removeRow(0);
    }
}

void Dialog::keyPressEvent(QKeyEvent *e)
{

    if(e->key()==Qt::Key_Escape)
        this->hide();
}

void Dialog::sendIFI(QTcpSocket* client)
{
    QByteArray block;
    QString type="IFI";
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

}
