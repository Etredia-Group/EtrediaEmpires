module;

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

module Network;
import utils;

ee_network::Lobby::Lobby() {
    if (SteamDatagramErrMsg error; !GameNetworkingSockets_Init(nullptr, error)) {
        LOG(CRITICAL, "Failed to initialize GameNetworkingSockets. %s", error);
        return;
    }

    steam = SteamNetworkingSockets();
}

ee_network::Lobby::~Lobby() {
    if (isConnected) {
        if (isHost)
            steam->CloseListenSocket(server);
        else steam->CloseConnection(
            connection,
            DISCONNECTION_STATUS_NETWORKING_DESTROY,
            "Disconnected due app networking close",
            false
        );
    }

    GameNetworkingSockets_Kill();
}

void ee_network::Lobby::Create(const std::uint16_t port) {
    SteamNetworkingIPAddr address; address.Clear();
    address.m_port = port;

    SteamNetworkingConfigValue_t config;

    server = steam->CreateListenSocketIP(address, 0, nullptr);

    char addressStr[SteamNetworkingIPAddr::k_cchMaxString];
    address.ToString(addressStr, sizeof(addressStr), true);
    LOG(INFO,"Server started: %hs", addressStr);
}

void ee_network::Lobby::Connect(const char *addressStr) {
    SteamNetworkingIPAddr address; address.ParseString(addressStr);
    connection = steam->ConnectByIPAddress(address, 0, nullptr);
}
