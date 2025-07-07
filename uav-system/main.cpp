#pragma execution_character_set(“utf-8”)
#include "widget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.resize(1925, 1300);
    w.show();

    return a.exec();
}
