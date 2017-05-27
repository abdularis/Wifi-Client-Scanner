#include "sniffer.h"
#include "rawsocket.h"
#include <QtDebug>
#include <QFile>
#include <QThread>

static QHash<QString, QString> mac_list_cache;

/*
 * Find device vendor of the given mac address
 */
QString find_mac_address(QString mac)
{
    if (mac.size() > 8)
        mac = mac.mid(0, 8);
    mac = mac.toLower();

    if (mac_list_cache.contains(mac)) {
        return mac_list_cache[mac];
    }

    QFile file(MAC_VENDOR_LIST_FILE_NAME);
    if (file.open(QIODevice::ReadOnly)) {
        qint64 read = 0;
        do {
            char buff[512];
            read = file.readLine(buff, sizeof(buff));

            QStringList sl = QString(buff).split('\t');
            if (sl.size() >= 2) {
                QString key = sl.at(0).simplified().toLower();
                QString val = sl.at(1).simplified();

                mac_list_cache[key] = val;

                if (key == mac) {
                    return val;
                }
            }

        } while (read > 0);
    }

    return "Unknown";
}


AccessPoint::AccessPoint(const QString &ssid, const QString &bssid)
    : ssid(ssid), bssid(bssid)
{
    vendor = find_mac_address(bssid);
}


AssocStation::AssocStation(const QString &mac, const AccessPoint &ap)
    : mac(mac), ap(ap)
{
    vendor = find_mac_address(mac);
}


WifiSniffer::WifiSniffer(const QString& iface, QObject *parent) :
    QObject(parent), mCurrChannel(1)
{
    mStarted = false;
    mIface = iface;
    mRS = NULL;
    mCurrChannel = 1;

    // change wifi channel regularly
    startTimer(CHANNEL_SWITCH_INTERVAL);
}

// see 802.11 frame spec
void WifiSniffer::onReadyRead()
{
    // ignore radiotap header
    QByteArray frame = mRS->read(4096).mid(36);

    int type = (frame.at(0) & 0xC) >> 2;
    int subtype = (frame.at(0) & 0xF0) >> 4;

    // type:0 subtype:8 = beacon frame
    if (type == 0 && subtype == 8) {

        QString bssid = parseMacAddress(frame.mid(16, 6));
        int ssidLen = (int) frame.at(37);
        QString ssid = QString::fromStdString(frame.mid(38, ssidLen).toStdString());

        if (!mAP.contains(bssid)) {
            mAP[bssid] = AccessPoint(ssid, bssid);
            emit accessPointAdded(mAP[bssid]);
        }

    }
    else if (type == 2) { // type:2 = data frame
        int toDs = frame.at(1) & 0x1;           // to distributed system flag
        int fromDs = (frame.at(1) & 0x2) >> 1;  // from distributed system flag

        // data from station to distributed system (ds) (through AP)
        if (toDs == 1 && fromDs == 0) {
            QString apBssid = parseMacAddress(frame.mid(4, 6));
            QString clientMac = parseMacAddress(frame.mid(10, 6));

            if (!mAssoc.contains(apBssid)) {
                if (mAP.contains(apBssid)) {
                    mAssoc[apBssid] = AssocStation(clientMac, mAP.value(apBssid));
                    emit assocStationAdded(mAssoc[apBssid]);
                }
            }

        }
    }

}

QVector<AccessPoint> WifiSniffer::getAPList() const
{
    QVector<AccessPoint> apv;
    for (AccessPoint ap : mAP)
        apv.push_back(ap);
    return apv;
}

QVector<AssocStation> WifiSniffer::getAssocStationList() const
{
    QVector<AssocStation> assoc;
    for (AssocStation as : mAssoc)
        assoc.push_back(as);
    return assoc;
}

void WifiSniffer::setInterface(QString iface)
{
    mIface = iface;
}

QString WifiSniffer::getInterface() const
{
    return mIface;
}

void WifiSniffer::clearData()
{
    mAP.clear();
    mAssoc.clear();
}

bool WifiSniffer::isStarted() const
{
    return mStarted;
}

void WifiSniffer::start()
{
    if (!mStarted) {
        mStarted = true;

        if (mRS != NULL && !mIface.isEmpty() && mIface != mRS->getInterface()) {
            mRS->close();
            delete mRS;
            mRS = NULL;
        }

        switchWifiMode(WIFI_MODE_MONITOR);
        if (mRS == NULL) {
            mRS = new RawSocket(mIface, this);
        }
        connect(mRS, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    }
}

void WifiSniffer::stop()
{
    if (mStarted) {
        mStarted = false;

        if (mRS != NULL) {
            switchWifiMode(WIFI_MODE_MANAGED);
            disconnect(mRS, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
        }
    }
}

QString WifiSniffer::parseMacAddress(const QByteArray &raw_data)
{
    if (raw_data.size() < 6)
        return "";
    return QString::asprintf("%02X:%02X:%02X:%02X:%02X:%02X",
                             (unsigned char) raw_data[0],
                             (unsigned char) raw_data[1],
                             (unsigned char) raw_data[2],
                             (unsigned char) raw_data[3],
                             (unsigned char) raw_data[4],
                             (unsigned char) raw_data[5]);
}


/*
 * Switch wifi interface card to specific wifi channel every CHANNEL_SWITCH_INTERVAL ms
 */
void WifiSniffer::timerEvent(QTimerEvent *ev)
{
    if (!mStarted) return;
    if (++mCurrChannel > WIFI_2_4_GHZ_CHANNEL_COUNT)
        mCurrChannel = 1;

    system(QString("iwconfig " + mRS->getInterface() + " channel " + QString::number(mCurrChannel))
           .toStdString().c_str());
    emit channelChanged(mCurrChannel);
}

void WifiSniffer::switchWifiMode(QString mode)
{
    QString msg = "Turning " + mIface + " into " + mode + " mode...";
    qDebug() << msg;

    system(QString("ifconfig " + mIface + " down").toStdString().c_str());
    system(QString("iwconfig " + mIface + " mode " + mode).toStdString().c_str());
    system(QString("ifconfig " + mIface + " up").toStdString().c_str());

    QThread::msleep(400);
}
