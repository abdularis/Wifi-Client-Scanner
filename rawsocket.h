#ifndef RAWSOCKET_H
#define RAWSOCKET_H

#include <QAbstractSocket>

class RawSocket : public QAbstractSocket
{
    Q_OBJECT

public:
    explicit RawSocket(const QString& net_iface, QObject *parent = 0);

    inline QString getInterface() const { return mNetIface; }

private:

    QString mNetIface;
};

#endif // RAWSOCKET_H
