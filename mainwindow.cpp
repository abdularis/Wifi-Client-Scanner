#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(const QString& iface, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mIface(iface)
{
    ui->setupUi(this);

    mWS = new WifiSniffer(mIface, this);

    connect(mWS, SIGNAL(accessPointAdded(AccessPoint)), this, SLOT(onAPAdded(AccessPoint)));
    connect(mWS, SIGNAL(assocStationAdded(AssocStation)), this, SLOT(onAssocStatAdded(AssocStation)));
    connect(mWS, SIGNAL(channelChanged(int)), this, SLOT(onChannelChanged(int)));

    mAPModel = new QStandardItemModel();
    mAssocModel = new QStandardItemModel();

    setupTableHeaders();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onAPAdded(const AccessPoint &ap)
{
    int row = mAPModel->rowCount();
    mAPModel->setItem(row, 0, new QStandardItem(ap.ssid));
    mAPModel->setItem(row, 1, new QStandardItem(ap.bssid));
    mAPModel->setItem(row, 2, new QStandardItem(ap.vendor));
}

void MainWindow::onAssocStatAdded(const AssocStation &as)
{
    int row = mAssocModel->rowCount();
    mAssocModel->setItem(row, 0, new QStandardItem(as.ap.ssid));
    mAssocModel->setItem(row, 1, new QStandardItem(as.ap.bssid));
    mAssocModel->setItem(row, 2, new QStandardItem(as.mac));
    mAssocModel->setItem(row, 3, new QStandardItem(as.vendor));
}

void MainWindow::onChannelChanged(int channel)
{
    ui->statusBar->showMessage("Channel " + QString::number(channel));
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    if (QMessageBox::question(this, "Exit?", "Are you sure want to exit?")
            == QMessageBox::Yes) {
        mWS->stop();
    }
    else {
        ev->ignore();
    }

}

void MainWindow::on_saveListAsBtn_clicked()
{
    QString fname = QFileDialog::getSaveFileName(this, "Save List to file");
    if (fname.isEmpty())
        return;

    QVector<AccessPoint> aps = mWS->getAPList();
    QVector<AssocStation> assoc = mWS->getAssocStationList();

    QFile file(fname);
    if (file.open(QIODevice::WriteOnly)) {
        QDateTime currDate = QDateTime::currentDateTime();
        QString strDate = "Date:\t" + currDate.toString("ddd dd MMM, yyyy") + "\n";

        file.write(strDate.toStdString().c_str());
        file.write("==============[  List of Access Point  ]==============\n");
        for (int i = 0; i < aps.size(); i++) {
            AccessPoint ap = aps.at(i);
            QString str = "\n---["+ QString::number(i+1) +"]---\n" +
                    "SSID   :" + ap.ssid    + "\n" +
                    "BSSID  :" + ap.bssid   + "\n" +
                    "Vendor :" + ap.vendor  + "\n";
            file.write(str.toStdString().c_str());
        }

        file.write("\n==============[  List of Associated Station (Client)  ]==============\n");
        for (int i = 0; i < assoc.size(); i++) {
            AssocStation as = assoc.at(i);
            QString str = "\n---["+ QString::number(i+1) +"]---\n" +
                    "AP     :" + as.ap.ssid     + "\n" +
                    "BSSID  :" + as.ap.bssid    + "\n" +
                    "MAC    :" + as.mac         + "\n" +
                    "Vendor :" + as.vendor      + "\n";
            file.write(str.toStdString().c_str());
        }

        file.close();
    }
}

void MainWindow::on_startToggleBtn_toggled(bool checked)
{
    if (checked) {
        mIface = ui->ifaceText->text().toLower();
        if (!mIface.isEmpty()) {
            ui->startToggleBtn->setText("Stop");
            ui->ifaceText->setEnabled(false);

            mWS->setInterface(mIface);
            mWS->start();
        }
    }
    else {
        ui->startToggleBtn->setText("Start");
        ui->ifaceText->setEnabled(true);
        mWS->stop();
    }
}

void MainWindow::on_clearBtn_clicked()
{
    mAPModel->clear();
    mAssocModel->clear();
    mWS->clearData();

    setupTableHeaders();
}

void MainWindow::setupTableHeaders()
{
    mAPModel->setHorizontalHeaderItem(0, new QStandardItem("SSID"));
    mAPModel->setHorizontalHeaderItem(1, new QStandardItem("BSSID"));
    mAPModel->setHorizontalHeaderItem(2, new QStandardItem("Vendor"));

    mAssocModel->setHorizontalHeaderItem(0, new QStandardItem("AP SSID"));
    mAssocModel->setHorizontalHeaderItem(1, new QStandardItem("AP BSSID"));
    mAssocModel->setHorizontalHeaderItem(2, new QStandardItem("Client MAC"));
    mAssocModel->setHorizontalHeaderItem(3, new QStandardItem("Client Vendor"));

    ui->ap_table->setModel(mAPModel);
    ui->ap_table->setColumnWidth(0, 250);
    ui->ap_table->setColumnWidth(1, 160);
    ui->ap_table->setColumnWidth(2, 280);

    ui->as_table->setModel(mAssocModel);
    ui->as_table->setColumnWidth(0, 250);
    ui->as_table->setColumnWidth(1, 160);
    ui->as_table->setColumnWidth(2, 160);
    ui->as_table->setColumnWidth(3, 280);
}
