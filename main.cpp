#include "MainWindow.h"
#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QIcon>
#include <QtGlobal>
#include <QByteArray>

int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("simple"));
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/pix/Quish.png"));
    MainWindow w;
    w.show();
    return a.exec();
}