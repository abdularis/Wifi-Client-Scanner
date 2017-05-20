#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if.h>

#include "rawsocket.h"

RawSocket::RawSocket(const QString& net_iface, QObject *parent)
    : QAbstractSocket(QAbstractSocket::UnknownSocketType, parent),
        mNetIface(net_iface)
{
    /*
     * Using linux raw socket
     */

    int sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_fd == -1) {
        printf("[!] Failed to create socket\n");
        return;
    }

    struct ifreq ifr;
    memset((void*)&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), mNetIface.toStdString().c_str());

    if (setsockopt(sock_fd, SOL_SOCKET, SO_BINDTODEVICE, (void*)&ifr, sizeof(ifr)) == -1) {
        printf("[!] Failed to bind to %s\n", mNetIface);
        return;
    }

    setSocketDescriptor(sock_fd);
}
