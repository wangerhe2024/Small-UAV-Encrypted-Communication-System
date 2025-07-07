#include "widget.h"
#include "ui_widget.h"
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QTimer>
#include <QProcess>
#include <QDebug>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 定时器每隔1秒更新一次文件
    tim = new QTimer();
    tim->setInterval(1000);
    connect(tim,SIGNAL(timeout()),this,SLOT(NumBig()));
    connect(tim,SIGNAL(timeout()),this,SLOT(QGCShowKeyUpdate()));
    connect(tim,SIGNAL(timeout()),this,SLOT(UAVShowKeyUpdate()));
    // 除了密钥更新部分外，其他部分30s后就可以不用更新到显示页面了，避免一直使用线程
    if (NumFlag < 60)
    {
        connect(tim,SIGNAL(timeout()),this,SLOT(QGCShowAuthDis()));
        connect(tim,SIGNAL(timeout()),this,SLOT(UAVShowAuthDis()));
    }
    connect(tim,SIGNAL(timeout()),this,SLOT(QGCShowVideoData()));
    // connect(tim,SIGNAL(timeout()),this,SLOT(UAVShowVideoData()));
    connect(tim,SIGNAL(timeout()),this,SLOT(QGCShowData()));
    // connect(tim,SIGNAL(timeout()),this,SLOT(UAVShowData()));
    tim->start();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::ShowFileContent(QString fn, int tf)
{
    QString fileName = fn;
    int TextFlag = tf;
    if(!fileName.isEmpty())
    {
        QFile *file=new QFile;
        file->setFileName(fileName);
        bool ok=file->open(QIODevice::ReadOnly);
        if(ok)
        {
            QTextStream in(file);
            switch(TextFlag){
                case 0:
                   ui->QGCKeyUpdateLog->setText(in.readAll());
                   break;
                case 1:
                   ui->UAVKeyUpdateLog->setText(in.readAll());
                   break;
                case 2:
                   ui->QGCAuthDisLog->setText(in.readAll());
                   break;
                case 3:
                   ui->UAVAuthDisLog->setText(in.readAll());
                   break;
                case 4:
                   ui->QGCVideoDeBefore->setText(in.readAll());
                   break;
                case 5:
                   ui->QGCVideoDeAfter->setText(in.readAll());
                   break;
                case 8:
                   ui->QGCDataDeBefore->setText(in.readAll());
                   break;
                case 9:
                   ui->QGCDataDeAfter->setText(in.readAll());
                   break;
                case 10:
                   ui->MAVLinkMsgId->setText(in.readAll());
                   break;
                default:
                   ui->QGCKeyUpdateLog->setText(in.readAll());
            }
            file->close();
            delete file;
        }
        else
        {
            // QMessageBox::information(this,"Error Message","Open File:"+file->errorString());
            return;
        }
    }
}

// 地面控制站密钥更新
void Widget::QGCShowKeyUpdate()
{
    QString fileName = "/home/newling/Zectun/luolin/ccs/log/KeyUpdate.log";
    ShowFileContent(fileName, 0);
}
// 小型无人机密钥更新
void Widget::UAVShowKeyUpdate()
{
    QProcess process;
    QString cmd = "rsync -av ubuntu@10.42.0.10:/opt/go/src/github.com/luolin/ccs/log/KeyUpdate.log /home/newling/Zectun/luolin/ccs/uavlog";
    process.start(cmd.toLatin1());
    process.waitForFinished();

    QString fileName = "/home/newling/Zectun/luolin/ccs/uavlog/KeyUpdate.log";
    ShowFileContent(fileName, 1);
}

// 地面控制站认证协商
void Widget::QGCShowAuthDis()
{
    QString fileName = "/home/newling/Zectun/luolin/ccs/log/AuthDis.log";
    ShowFileContent(fileName, 2);
}
// 小型无人机认证协商
void Widget::UAVShowAuthDis()
{
    QProcess process;
    QString cmd = "rsync -av ubuntu@10.42.0.10:/opt/go/src/github.com/luolin/ccs/log/AuthDis.log /home/newling/Zectun/luolin/ccs/uavlog";
    process.start(cmd.toLatin1());
    process.waitForFinished();

    QString fileName = "/home/newling/Zectun/luolin/ccs/uavlog/AuthDis.log";
    ShowFileContent(fileName, 3);
}

// 地面控制站图传数据解密
void Widget::QGCShowVideoData()
{
    // 解密前
    QString fileName1 = "/home/newling/Zectun/luolin/ccs/log/before_video.txt";
    ShowFileContent(fileName1, 4);
    // 解密后
    QString fileName2 = "/home/newling/Zectun/luolin/ccs/log/after_video.txt";
    ShowFileContent(fileName2, 5);
}
// 小型无人机图传数据加密
/*void Widget::UAVShowVideoData()
{
    QProcess process1;
    QString cmd1 = "rsync -av pi@10.0.2.100:/opt/go/src/github.com/luolin/ccs/log/before_video.txt /home/newling/Zectun/luolin/ccs/uavlog";
    process1.start(cmd1.toLatin1());
    process1.waitForFinished();

    QProcess process2;
    QString cmd2 = "rsync -av pi@10.0.2.100:/opt/go/src/github.com/luolin/ccs/log/after_video.txt /home/newling/Zectun/luolin/ccs/uavlog";
    process2.start(cmd2.toLatin1());
    process2.waitForFinished();

    // 加密前
    QString fileName1 = "/home/newling/Zectun/luolin/ccs/uavlog/before_video.txt";
    ShowFileContent(fileName1, 6);

    // 加密后
    QString fileName2 = "/home/newling/Zectun/luolin/ccs/uavlog/after_video.txt";
    ShowFileContent(fileName2, 7);
}*/

// 地面控制站数传数据解密
void Widget::QGCShowData()
{
    // 解密前
    QString fileName1 = "/home/newling/Zectun/luolin/ccs/log/before_data.txt";
    ShowFileContent(fileName1, 8);
    // 解密后
    QString fileName2 = "/home/newling/Zectun/luolin/ccs/log/after_data.txt";
    ShowFileContent(fileName2, 9);
    QString fileName3 = "/home/newling/Zectun/luolin/ccs/log/mavlink_id.txt";
    ShowFileContent(fileName3, 10);
}
// 小型无人机数传数据加密
/*void Widget::UAVShowData()
{
    QProcess process1;
    QString cmd1 = "rsync -av pi@10.0.2.100:/opt/go/src/github.com/luolin/ccs/log/before_data.txt /home/newling/Zectun/luolin/ccs/uavlog";
    process1.start(cmd1.toLatin1());
    process1.waitForFinished();

    QProcess process2;
    QString cmd2 = "rsync -av pi@10.0.2.100:/opt/go/src/github.com/luolin/ccs/log/after_data.txt /home/newling/Zectun/luolin/ccs/uavlog";
    process2.start(cmd2.toLatin1());
    process2.waitForFinished();

    // 加密前
    QString fileName1 = "/home/newling/Zectun/luolin/ccs/uavlog/before_data.txt";
    ShowFileContent(fileName1, 10);

    // 加密后
    QString fileName2 = "/home/newling/Zectun/luolin/ccs/uavlog/after_data.txt";
    ShowFileContent(fileName2, 11);
}*/

// 地面控制站密钥更新栏自动到底部
void Widget::on_QGCKeyUpdateLog_textChanged()
{
    ui->QGCKeyUpdateLog->moveCursor(QTextCursor::End);
}
// 无人机密钥更新栏自动到底部
void Widget::on_UAVKeyUpdateLog_textChanged()
{
    ui->UAVKeyUpdateLog->moveCursor(QTextCursor::End);
}

void Widget::NumBig()
{
    NumFlag++;
}

/*void Widget::on_CheckQgcTimeCost_clicked()
{
    float QgcTime[4];
    int i = 0;
    QFile file("/home/newling/Zectun/luolin/ccs/test/TimeCost.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while (!file.atEnd())
        {
            QByteArray line = file.readLine();
            QString str(line);
            QString t = QString::asprintf("%.2f", str.toFloat());
            QgcTime[i] = t.toFloat();
            i++;
            // qDebug() << str;
         }
         file.close();
    }
    QBarSet *set0 = new QBarSet("加密");
    QBarSet *set1 = new QBarSet("解密");

    *set0 << QgcTime[0] << QgcTime[2];
    *set1 << QgcTime[1] << QgcTime[3];

    QBarSeries *series = new QBarSeries();
    series->append(set0);
    series->append(set1);

    QChart *chart = new QChart();
    chart->addSeries(series);
    // chart->setTitle("地面控制站加解密时间损耗  单位:微秒");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QStringList categories;
    categories << "SM4" << "ZUC";
    QBarCategoryAxis *axis = new QBarCategoryAxis();
    axis->append(categories);
    chart->createDefaultAxes();
    chart->setAxisX(axis, series);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    ui->graphicsViewQGC->setChart(chart);
}*/

void Widget::on_Close_clicked()
{
    this->close();
}

/*void Widget::on_CheckUavTimeCost_clicked()
{
    QProcess process;
    QString cmd = "rsync -av pi@10.0.2.100:/opt/go/src/github.com/luolin/ccs/test/TimeCost.txt /home/newling/Zectun/luolin/ccs/uavlog";
    process.start(cmd.toLatin1());
    process.waitForFinished();

    float QgcTime[4];
    int i = 0;
    QFile file("/home/newling/Zectun/luolin/ccs/uavlog/TimeCost.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while (!file.atEnd())
        {
            QByteArray line = file.readLine();
            QString str(line);
            QString t = QString::asprintf("%.2f", str.toFloat());
            QgcTime[i] = t.toFloat();
            i++;
            // qDebug() << str;
         }
         file.close();
    }
    QBarSet *set0 = new QBarSet("加密");
    QBarSet *set1 = new QBarSet("解密");

    *set0 << QgcTime[0] << QgcTime[2];
    *set1 << QgcTime[1] << QgcTime[3];

    QBarSeries *series = new QBarSeries();
    series->append(set0);
    series->append(set1);

    QChart *chart = new QChart();
    chart->addSeries(series);
    // chart->setTitle("地面控制站加解密时间损耗  单位:微秒");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QStringList categories;
    categories << "SM4" << "ZUC";
    QBarCategoryAxis *axis = new QBarCategoryAxis();
    axis->append(categories);
    chart->createDefaultAxes();
    chart->setAxisX(axis, series);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    ui->graphicsViewUAV->setChart(chart);
}*/
