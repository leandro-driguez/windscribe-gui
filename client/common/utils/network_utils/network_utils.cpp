#include "network_utils.h"

#include "../utils.h"

#if defined Q_OS_WIN
#include "network_utils_win.h"
#elif defined Q_OS_MAC
#include "network_utils_mac.h"
#elif defined Q_OS_LINUX
#include "network_utils_linux.h"
#endif

QString NetworkUtils::generateRandomMacAddress()
{
    QString s;

    for (int i = 0; i < 6; i++) {
        char buf[256];
        int tp = Utils::generateIntegerRandom(0, 255);

        // Lowest bit in first byte must not be 1 ( 0 - Unicast, 1 - multicast )
        // 2nd lowest bit in first byte must be 1 ( 0 - device, 1 - locally administered mac address )
        if ( i == 0) {
            tp |= 0x02;
            tp &= 0xFE;
        }

        sprintf(buf, "%s%X", tp < 16 ? "0" : "", tp);
        s += QString::fromStdString(buf);
    }
    return s;
}

QString NetworkUtils::formatMacAddress(QString macAddress)
{
    // WS_ASSERT(macAddress.length() == 12);

    QString formattedMac = QString("%1:%2:%3:%4:%5:%6").arg(macAddress.mid(0,2))
                               .arg(macAddress.mid(2,2))
                               .arg(macAddress.mid(4,2))
                               .arg(macAddress.mid(6,2))
                               .arg(macAddress.mid(8,2))
                               .arg(macAddress.mid(10,2));

    return formattedMac;
}

QVector<types::NetworkInterface> NetworkUtils::interfacesExceptOne(const QVector<types::NetworkInterface> &interfaces, const types::NetworkInterface &exceptInterface)
{
    auto differentInterfaceName = [&exceptInterface](const types::NetworkInterface &ni)
    {
        return ni.interfaceName != exceptInterface.interfaceName;
    };

    QVector<types::NetworkInterface> resultInterfaces;
    std::copy_if(interfaces.begin(), interfaces.end(),  std::back_inserter(resultInterfaces), differentInterfaceName);
    return resultInterfaces;
}

bool NetworkUtils::pingWithMtu(const QString &url, int mtu)
{
#ifdef Q_OS_WIN
    return NetworkUtils_win::pingWithMtu(url, mtu);
#elif defined Q_OS_MAC
    return NetworkUtils_mac::pingWithMtu(url, mtu);
#elif defined Q_OS_LINUX
    // This method is not currently used on Linux.
    Q_UNUSED(url)
    Q_UNUSED(mtu)
    return false;
#endif
}

QString NetworkUtils::getLocalIP()
{
#ifdef Q_OS_WIN
    return NetworkUtils_win::getLocalIP();
#elif defined Q_OS_MAC
    return NetworkUtils_mac::getLocalIP();
#elif defined Q_OS_LINUX
    return NetworkUtils_linux::getLocalIP();
#endif
}

QString NetworkUtils::networkInterfacesToString(const QVector<types::NetworkInterface> &networkInterfaces, bool includeIndex)
{
    QString adapters;
    for (const auto &networkInterface : networkInterfaces) {
        adapters += networkInterface.interfaceName;
        if (includeIndex) {
            adapters += QString(" (%1)").arg(networkInterface.interfaceIndex);
        }
        adapters += ", ";
    }

    if (!adapters.isEmpty()) {
        // Remove the trailing delimiter.
        adapters.chop(2);
    }

    return adapters;
}
