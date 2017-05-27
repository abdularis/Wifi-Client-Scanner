#include <unistd.h>
#include <sys/types.h>

#include <QApplication>
#include <QMessageBox>

#include "mainwindow.h"
#include "sniffer.h"

int main(int argc, char *argv[])
{
    uid_t uid = getuid();
    if (uid != 0) {
        printf("Please run this program as root.\n");
        return 1;
    }

    QApplication a(argc, argv);

    MainWindow* w = new MainWindow(argv[1]);
    w->show();

    return a.exec();
}
