#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include "utils/protobuf_includes.h"

const int MTU_OFFSET_OPENVPN = 40;
const int MTU_OFFSET_IKEV2 = 80;
const int MTU_OFFSET_WG = 80;

enum CONNECTION_ERROR { // common
                        NO_CONNECT_ERROR,
                        AUTH_ERROR,
                        COULD_NOT_FETCH_CREDENTAILS,
                        LOCATION_NOT_EXIST,
                        LOCATION_NO_ACTIVE_NODES,
                        CANT_RESOLVE_HOSTNAME,
                        CONNECTION_BLOCKED,
                        // openvpn
                        NO_OPENVPN_SOCKET, CANT_RUN_OPENVPN, CANNOT_ALLOCATE_TUN_TAP, NO_INSTALLED_TUN_TAP, ALL_TAP_IN_USE,
                        CANNOT_CONNECT_TO_SERVICE_PIPE, CANNOT_WRITE_TO_SERVICE_PIPE,
                        NO_AVAILABLE_PORT, PROXY_AUTH_ERROR, UDP_CANT_ASSIGN, WINTUN_OVER_CAPACITY,  CONNECTED_ERROR, INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS,
                        UDP_NO_BUFFER_SPACE, UDP_NETWORK_DOWN, TCP_ERROR, CANNOT_OPEN_CUSTOM_CONFIG,
                        // ikev2
                        IKEV_FAILED_TO_CONNECT,
                        IKEV_NOT_FOUND_WIN, IKEV_FAILED_SET_ENTRY_WIN, IKEV_FAILED_MODIFY_HOSTS_WIN,
                        IKEV_NETWORK_EXTENSION_NOT_FOUND_MAC, IKEV_FAILED_SET_KEYCHAIN_MAC, IKEV_FAILED_START_MAC, IKEV_FAILED_LOAD_PREFERENCES_MAC, IKEV_FAILED_SAVE_PREFERENCES_MAC,
                        // wireguard
                        WIREGUARD_CONNECTION_ERROR,
                        // emergency
                        EMERGENCY_FAILED_CONNECT
                        };

enum DISCONNECT_REASON { DISCONNECTED_ITSELF, DISCONNECTED_BY_USER, DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED, DISCONNECTED_WITH_ERROR };

enum PROXY_OPTION { PROXY_OPTION_NONE, PROXY_OPTION_AUTODETECT, PROXY_OPTION_HTTP, PROXY_OPTION_SOCKS };

enum INIT_HELPER_RET { INIT_HELPER_SUCCESS, INIT_HELPER_FAILED };

enum LOGIN_RET { LOGIN_SUCCESS, LOGIN_NO_API_CONNECTIVITY, LOGIN_NO_CONNECTIVITY, LOGIN_INCORRECT_JSON,
                 LOGIN_BAD_USERNAME, LOGIN_PROXY_AUTH_NEED, LOGIN_SSL_ERROR, LOGIN_MISSING_CODE2FA,
                 LOGIN_BAD_CODE2FA };

enum LOGIN_MESSAGE { LOGIN_MESSAGE_NONE, LOGIN_MESSAGE_TRYING_BACKUP1, LOGIN_MESSAGE_TRYING_BACKUP2 };

enum SERVER_API_RET_CODE { SERVER_RETURN_SUCCESS, SERVER_RETURN_NETWORK_ERROR, SERVER_RETURN_INCORRECT_JSON, SERVER_RETURN_BAD_USERNAME,
                           SERVER_RETURN_PROXY_AUTH_FAILED, SERVER_RETURN_SSL_ERROR, SERVER_RETURN_API_NOT_READY,
                           SERVER_RETURN_MISSING_CODE2FA, SERVER_RETURN_BAD_CODE2FA };

enum ENGINE_INIT_RET_CODE { ENGINE_INIT_SUCCESS, ENGINE_INIT_HELPER_FAILED, ENGINE_INIT_BFE_SERVICE_FAILED };

enum CONNECT_STATE { CONNECT_STATE_DISCONNECTED, CONNECT_STATE_CONNECTED, CONNECT_STATE_CONNECTING,
                     CONNECT_STATE_DISCONNECTING};

enum PROXY_SHARING_TYPE { PROXY_SHARING_HTTP, PROXY_SHARING_SOCKS };

enum ORDER_LOCATIONS_TYPE { ORDER_LOCATIONS_BY_GEOGRAPHY, ORDER_LOCATIONS_BY_ALPHABETICALLY, ORDER_LOCATIONS_BY_LATENCY };

enum DNS_POLICY_TYPE { DNS_TYPE_OS_DEFAULT = 0, DNS_TYPE_OPEN_DNS, DNS_TYPE_CLOUDFLARE, DNS_TYPE_GOOGLE};

QString loginRetToString(LOGIN_RET ret);
ProtoTypes::LoginError loginRetToProtobuf(LOGIN_RET ret);
QString dnsPolicyTypeToString(DNS_POLICY_TYPE d);

#endif // TYPES_H


