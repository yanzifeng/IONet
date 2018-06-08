#include "ionetdialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    IONetDialog w;
    w.show();

    return a.exec();
}
