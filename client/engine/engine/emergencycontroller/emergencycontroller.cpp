#include "emergencycontroller.h"
#include "utils/ws_assert.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/connectionmanager/openvpnconnection.h"
#include "utils/hardcodedsettings.h"
#include "utils/dga_library.h"
#include "engine/dnsresolver/dnsrequest.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"
#include <QFile>
#include <QCoreApplication>
#include "utils/extraconfig.h"
#include "types/global_consts.h"

#include <random>

#ifdef Q_OS_MAC
    #include "utils/macutils.h"
    #include "engine/helper/helper_mac.h"
#endif

EmergencyController::EmergencyController(QObject *parent, IHelper *helper) : QObject(parent),
    helper_(helper),
    state_(STATE_DISCONNECTED)
{
    QFile file(":/resources/ovpn/emergency.ovpn");
    if (file.open(QIODevice::ReadOnly))
    {
        ovpnConfig_ = file.readAll();
        file.close();
    }
    else
    {
        qCDebug(LOG_EMERGENCY_CONNECT) << "Failed load emergency.ovpn from resources";
    }

     connector_ = new OpenVPNConnection(this, helper_);
     connect(connector_, &OpenVPNConnection::connected, this, &EmergencyController::onConnectionConnected, Qt::QueuedConnection);
     connect(connector_, &OpenVPNConnection::disconnected, this, &EmergencyController::onConnectionDisconnected, Qt::QueuedConnection);
     connect(connector_, &OpenVPNConnection::reconnecting, this, &EmergencyController::onConnectionReconnecting, Qt::QueuedConnection);
     connect(connector_, &OpenVPNConnection::error, this, &EmergencyController::onConnectionError, Qt::QueuedConnection);

     makeOVPNFile_ = new MakeOVPNFile();
}

EmergencyController::~EmergencyController()
{
    SAFE_DELETE(connector_);
    SAFE_DELETE(makeOVPNFile_);
}

void EmergencyController::clickConnect(const types::ProxySettings &proxySettings)
{
    WS_ASSERT(state_ == STATE_DISCONNECTED);
    state_= STATE_CONNECTING_FROM_USER_CLICK;

    proxySettings_ = proxySettings;

    QString hashedDomain = "econnect." + HardcodedSettings::instance().generateDomain();
    qCDebug(LOG_EMERGENCY_CONNECT) << "Generated hashed domain for emergency connect:" << hashedDomain;

    DnsRequest *dnsRequest = new DnsRequest(this, hashedDomain, DnsServersConfiguration::instance().getCurrentDnsServers());
    connect(dnsRequest, &DnsRequest::finished, this,&EmergencyController::onDnsRequestFinished);
    dnsRequest->lookup();
}

void EmergencyController::clickDisconnect()
{
    WS_ASSERT(state_ == STATE_CONNECTING_FROM_USER_CLICK || state_ == STATE_CONNECTED  ||
             state_ == STATE_DISCONNECTING_FROM_USER_CLICK || state_ == STATE_DISCONNECTED);

    if (state_ != STATE_DISCONNECTING_FROM_USER_CLICK)
    {
        state_ = STATE_DISCONNECTING_FROM_USER_CLICK;
        qCDebug(LOG_EMERGENCY_CONNECT) << "ConnectionManager::clickDisconnect()";
        if (connector_)
        {
            connector_->startDisconnect();
        }
        else
        {
            state_ = STATE_DISCONNECTED;
            emit disconnected(DISCONNECTED_BY_USER);
        }
    }
}

bool EmergencyController::isDisconnected()
{
    if (connector_)
    {
        return connector_->isDisconnected();
    }
    else
    {
        return true;
    }
}

void EmergencyController::blockingDisconnect()
{
    if (connector_)
    {
        if (!connector_->isDisconnected())
        {
            connector_->blockSignals(true);
            QElapsedTimer elapsedTimer;
            elapsedTimer.start();
            connector_->startDisconnect();
            while (!connector_->isDisconnected())
            {
                QThread::msleep(1);
                qApp->processEvents();

                if (elapsedTimer.elapsed() > 10000)
                {
                    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::blockingDisconnect() delay more than 10 seconds";
                    connector_->startDisconnect();
                    break;
                }
            }
            connector_->blockSignals(false);
            doMacRestoreProcedures();
            state_ = STATE_DISCONNECTED;
        }
    }
}

const AdapterGatewayInfo &EmergencyController::getVpnAdapterInfo() const
{
    WS_ASSERT(state_ == STATE_CONNECTED); // make sense only in connected state
    return vpnAdapterInfo_;
}

void EmergencyController::setPacketSize(types::PacketSize ps)
{
    packetSize_ = ps;
}

void EmergencyController::onDnsRequestFinished()
{
    DnsRequest *dnsRequest = qobject_cast<DnsRequest *>(sender());
    WS_ASSERT(dnsRequest != nullptr);

    attempts_.clear();

    if (!dnsRequest->isError())
    {
        qCDebug(LOG_EMERGENCY_CONNECT) << "DNS resolved:" << dnsRequest->ips();

        // generate connect attempts array
        std::vector<QString> randomVecIps;
        for (const QString &ip :  dnsRequest->ips())
        {
            randomVecIps.push_back(ip);
        }

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(randomVecIps.begin(), randomVecIps.end(), rd);

        for (std::vector<QString>::iterator it = randomVecIps.begin(); it != randomVecIps.end(); ++it)
        {
            CONNECT_ATTEMPT_INFO info1;
            info1.ip = *it;
            info1.port = 443;
            info1.protocol = "udp";

            CONNECT_ATTEMPT_INFO info2;
            info2.ip = *it;
            info2.port = 443;
            info2.protocol = "tcp";

            attempts_ << info1;
            attempts_ << info2;
        }

        addRandomHardcodedIpsToAttempts();
    }
    else
    {
        qCDebug(LOG_EMERGENCY_CONNECT) << "DNS resolve failed";
        addRandomHardcodedIpsToAttempts();
    }
    doConnect();
    dnsRequest->deleteLater();
}

void EmergencyController::onConnectionConnected(const AdapterGatewayInfo &connectionAdapterInfo)
{
    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::onConnectionConnected(), state_ =" << state_;

    vpnAdapterInfo_ = connectionAdapterInfo;
    qCDebug(LOG_CONNECTION) << "VPN adapter and gateway:" << vpnAdapterInfo_.makeLogString();

    state_ = STATE_CONNECTED;
    emit connected();
}

void EmergencyController::onConnectionDisconnected()
{
    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::onConnectionDisconnected(), state_ =" << state_;

    doMacRestoreProcedures();

    switch (state_)
    {
        case STATE_DISCONNECTING_FROM_USER_CLICK:
        case STATE_CONNECTED:
            state_ = STATE_DISCONNECTED;
            emit disconnected(DISCONNECTED_BY_USER);
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
        case STATE_ERROR_DURING_CONNECTION:
            if (!attempts_.empty())
            {
                state_ = STATE_CONNECTING_FROM_USER_CLICK;
                doConnect();
            }
            else
            {
                emit errorDuringConnection(CONNECT_ERROR::EMERGENCY_FAILED_CONNECT);
                state_ = STATE_DISCONNECTED;
            }
            break;
        default:
            WS_ASSERT(false);
    }
}

void EmergencyController::onConnectionReconnecting()
{
    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::onConnectionReconnecting(), state_ =" << state_;

    switch (state_)
    {
        case STATE_CONNECTED:
            state_ = STATE_DISCONNECTED;
            emit disconnected(DISCONNECTED_BY_USER);
            break;
        case STATE_CONNECTING_FROM_USER_CLICK:
            state_ = STATE_ERROR_DURING_CONNECTION;
            connector_->startDisconnect();
            break;
        default:
            WS_ASSERT(false);
    }
}

void EmergencyController::onConnectionError(CONNECT_ERROR err)
{
    qCDebug(LOG_EMERGENCY_CONNECT) << "EmergencyController::onConnectionError(), err =" << err;

    connector_->startDisconnect();
    if (err == CONNECT_ERROR::AUTH_ERROR
            || err == CONNECT_ERROR::CANT_RUN_OPENVPN
            || err == CONNECT_ERROR::NO_OPENVPN_SOCKET
            || err == CONNECT_ERROR::NO_INSTALLED_TUN_TAP
            || err == CONNECT_ERROR::ALL_TAP_IN_USE)
    {
        // emit error in disconnected event
        state_ = STATE_ERROR_DURING_CONNECTION;
    }
    else if (err == CONNECT_ERROR::UDP_CANT_ASSIGN
             || err == CONNECT_ERROR::UDP_NO_BUFFER_SPACE
             || err == CONNECT_ERROR::UDP_NETWORK_DOWN
             || err == CONNECT_ERROR::WINTUN_OVER_CAPACITY
             || err == CONNECT_ERROR::TCP_ERROR
             || err == CONNECT_ERROR::CONNECTED_ERROR
             || err == CONNECT_ERROR::INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS)
    {
        if (state_ == STATE_CONNECTED)
        {
            state_ = STATE_DISCONNECTING_FROM_USER_CLICK;
            connector_->startDisconnect();
        }
        else
        {
            state_ = STATE_ERROR_DURING_CONNECTION;
            connector_->startDisconnect();
        }

        // bIgnoreConnectionErrors_ need to prevent handle multiple error messages from openvpn
        /*if (!bIgnoreConnectionErrors_)
        {
            bIgnoreConnectionErrors_ = true;

            if (!checkFails())
            {
                if (state_ != STATE_RECONNECTING)
                {
                    emit reconnecting();
                    state_ = STATE_RECONNECTING;
                    startReconnectionTimer();
                }
                if (err == INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS)
                {
                    bNeedResetTap_ = true;
                }
                connector_->startDisconnect();
            }
            else
            {
                state_ = STATE_AUTO_DISCONNECT;
                connector_->startDisconnect();
                emit showFailedAutomaticConnectionMessage();
            }
        }*/
    }
    else
    {
        qCDebug(LOG_EMERGENCY_CONNECT) << "Unknown error from openvpn: " << err;
    }
}

void EmergencyController::doConnect()
{
    defaultAdapterInfo_ = AdapterGatewayInfo::detectAndCreateDefaultAdapterInfo();
    qCDebug(LOG_CONNECTION) << "Default adapter and gateway:" << defaultAdapterInfo_.makeLogString();

    WS_ASSERT(!attempts_.empty());
    CONNECT_ATTEMPT_INFO attempt = attempts_[0];
    attempts_.removeFirst();

    int mss = 0;
    if (!packetSize_.isAutomatic)
    {
        bool advParamsOpenVpnExists = false;
        int openVpnOffset = ExtraConfig::instance().getMtuOffsetOpenVpn(advParamsOpenVpnExists);
        if (!advParamsOpenVpnExists) openVpnOffset = MTU_OFFSET_OPENVPN;

        mss = packetSize_.mtu - openVpnOffset;

        if (mss <= 0)
        {
            mss = 0;
            qCDebug(LOG_PACKET_SIZE) << "Using default MSS - OpenVpn EmergencyController MSS too low: " << mss;
        }
        else
        {
            qCDebug(LOG_PACKET_SIZE) << "OpenVpn EmergencyController MSS: " << mss;
        }
    }
    else
    {
        qCDebug(LOG_PACKET_SIZE) << "Packet size mode auto - using default MSS (EmergencyController)";
    }


    bool bOvpnSuccess = makeOVPNFile_->generate(ovpnConfig_, attempt.ip, types::Protocol::fromString(attempt.protocol), attempt.port, 0, mss, defaultAdapterInfo_.gateway(), "", "");
    if (!bOvpnSuccess )
    {
        qCDebug(LOG_EMERGENCY_CONNECT) << "Failed create ovpn config";
        WS_ASSERT(false);
        return;
    }

    qCDebug(LOG_EMERGENCY_CONNECT) << "Connecting to IP:" << attempt.ip << " protocol:" << attempt.protocol << " port:" << attempt.port;
    DgaLibrary dga;
    if (dga.load()) {
        connector_->startConnect(makeOVPNFile_->config(), "", "", dga.getParameter(PAR_EMERGENCY_USERNAME), dga.getParameter(PAR_EMERGENCY_PASSWORD), proxySettings_, nullptr, false, false, false, QString());
        lastIp_ = attempt.ip;
    } else {
        qCDebug(LOG_EMERGENCY_CONNECT) << "No dga found";
        WS_ASSERT(false);
        return;
    }
}

void EmergencyController::doMacRestoreProcedures()
{
#ifdef Q_OS_MAC
    Helper_mac *helper_mac = dynamic_cast<Helper_mac *>(helper_);
    helper_mac->deleteRoute(lastIp_, 32, defaultAdapterInfo_.gateway());
    RestoreDNSManager_mac::restoreState(helper_);
#endif
}

void EmergencyController::addRandomHardcodedIpsToAttempts()
{
    DgaLibrary dga;
    QStringList ips;
    if (dga.load())
        ips << dga.getParameter(PAR_EMERGENCY_IP1) << dga.getParameter(PAR_EMERGENCY_IP2);

    std::vector<QString> randomVecIps;
    for (const QString &ip : ips)
    {
        randomVecIps.push_back(ip);
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(randomVecIps.begin(), randomVecIps.end(), rd);

    for (std::vector<QString>::iterator it = randomVecIps.begin(); it != randomVecIps.end(); ++it)
    {
        CONNECT_ATTEMPT_INFO info;
        info.ip = *it;
        info.port = 1194;
        info.protocol = "udp";

        attempts_ << info;
    }
}
