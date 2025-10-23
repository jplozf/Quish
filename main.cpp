#include "MainWindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/pix/Quish.png"));
    MainWindow w;
    w.show();
    return a.exec();
}
