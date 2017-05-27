#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>

#include "sniffer.h"

#define WIFI_MODE_MONITOR "monitor"
#define WIFI_MODE_MANAGED "managed"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString& iface = QString(), QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* ev);

private slots:
    void onAPAdded(const AccessPoint& ap);
    void onAssocStatAdded(const AssocStation& as);
    void onChannelChanged(int channel);

    void on_saveListAsBtn_clicked();

    void on_startToggleBtn_toggled(bool checked);

    void on_clearBtn_clicked();

private:
    void setupTableHeaders();

    Ui::MainWindow *ui;

    WifiSniffer* mWS;

    QStandardItemModel* mAPModel;
    QStandardItemModel* mAssocModel;

    QString mIface;
};

#endif // MAINWINDOW_H
