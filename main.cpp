#include "mergeTable.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    mergeTable w;
    w.show();
    return a.exec();
}
