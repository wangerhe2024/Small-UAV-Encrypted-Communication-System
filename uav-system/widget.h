#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

// using namespace QT_CHARTS_NAMESPACE;

namespace Ui {
    class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    int NumFlag = 0;
    ~Widget();

private slots:
    // void on_CheckKeyBtn_clicked();
    void QGCShowKeyUpdate();
    void UAVShowKeyUpdate();
    void QGCShowAuthDis();
    void UAVShowAuthDis();
    void QGCShowVideoData();
    // void UAVShowVideoData();
    void QGCShowData();
    // void UAVShowData();
    void on_QGCKeyUpdateLog_textChanged();
    void on_UAVKeyUpdateLog_textChanged();
    void NumBig();
    // void on_CheckQgcTimeCost_clicked();
    void on_Close_clicked();
    // void on_CheckUavTimeCost_clicked();

private:
    Ui::Widget *ui;
    QTimer *tim;
    void ShowFileContent(QString fn, int tf);
};

#endif // WIDGET_H
