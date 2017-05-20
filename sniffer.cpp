#include "sniffer.h"
#include "rawsocket.h"
#include <QtDebug>
#include <QFile>

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

    QFile file("mlist/db");
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
    mRS = new RawSocket(iface, this);
    connect(mRS, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

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
        int ssid_len = (int) frame.at(37);
        QString ssid = QString::fromStdString(frame.mid(38, ssid_len).toStdString());

        if (!mAP.contains(bssid)) {
            mAP[bssid] = AccessPoint(ssid, bssid);
            emit accessPointAdded(mAP[bssid]);
        }

    }
    // type:2 = data frame
    else if (type == 2) {
        int to_ds = frame.at(1) & 0x1;          // to distributed system flag
        int frm_ds = (frame.at(1) & 0x2) >> 1;  // from distributed system flag

        // data from station to distributed system (ds) (through AP)
        if (to_ds == 1 && frm_ds == 0) {
            QString bssid = parseMacAddress(frame.mid(4, 6));
            QString sa = parseMacAddress(frame.mid(10, 6));

            if (!mAssoc.contains(bssid)) {
                if (mAP.contains(bssid)) {
                    mAssoc[bssid] = AssocStation(sa, mAP.value(bssid));
                    emit assocStationAdded(mAssoc[bssid]);
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
    if (++mCurrChannel > 14)
        mCurrChannel = 1;

    system(QString("iwconfig " + mRS->getInterface() + " channel " + QString::number(mCurrChannel))
           .toStdString().c_str());
    emit channelChanged(mCurrChannel);
}
