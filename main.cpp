#include "amurcore.h"
#include <QApplication>
#include <QVector>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AmurCore w;
    w.show();

    return a.exec();
}
