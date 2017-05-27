#ifndef SNIFFER_H
#define SNIFFER_H

#include <QObject>
#include <QHash>

#define MAC_VENDOR_LIST_FILE_NAME "vendorlist/list"
#define WIFI_2_4_GHZ_CHANNEL_COUNT 11

#define WIFI_MODE_MONITOR "monitor"
#define WIFI_MODE_MANAGED "managed"


/*
 * Forward declaration for RawSocket class
 */
class RawSocket;

/*
 * Store information about detected access point
 */
class AccessPoint
{
public:
    AccessPoint(const QString& ssid = QString(), const QString& bssid = QString());

    bool operator==(const AccessPoint& other) {
        return ssid == other.ssid && bssid == other.bssid;
    }

    QString ssid;
    QString bssid;
    QString vendor;
};


/*
 * Store information about station (client) associated with particular access point
 */
class AssocStation
{
public:
    AssocStation(const QString& mac = QString(), const AccessPoint& ap = QString());

    QString mac;
    QString vendor;
    AccessPoint ap;
};

class WifiSniffer : public QObject
{
    Q_OBJECT

public:
    static const int CHANNEL_SWITCH_INTERVAL = 500;

    explicit WifiSniffer(const QString& iface, QObject *parent = 0);

    QVector<AccessPoint> getAPList() const;
    QVector<AssocStation> getAssocStationList() const;

    bool isStarted() const;

    void setInterface(QString iface);
    QString getInterface() const;

    void clearData();

public slots:
    void start();
    void stop();

signals:
    void accessPointAdded(const AccessPoint& ap);
    void assocStationAdded(const AssocStation& asoc);
    void channelChanged(int channel);

protected:
    void timerEvent(QTimerEvent* ev);

private slots:
    void onReadyRead();

private:
    QString parseMacAddress(const QByteArray& raw_data);
    void switchWifiMode(QString mode);

    bool mStarted;
    QString mIface;
    RawSocket* mRS;
    QHash<QString, AccessPoint> mAP;
    QHash<QString, AssocStation> mAssoc;

    int mCurrChannel;
};

#endif // SNIFFER_H
