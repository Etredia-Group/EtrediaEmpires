module;

#include <steam/isteamnetworkingsockets.h>

export module Network;
import std;

export namespace ee_network {

    enum DisconnectionStatus {
        DISCONNECTION_STATUS_NETWORKING_DESTROY = k_ESteamNetConnectionEnd_AppException_Min,

        DISCONNECTION_STATUS_NONE = k_ESteamNetConnectionEnd_AppException_Max
    };

    class Lobby {
    public:
        explicit Lobby();
        ~Lobby();

        void Create(std::uint16_t port);
        void Connect(const char *addressStr);
        void Disconnect();

    private:
        bool isConnected = false;
        bool isHost = false;

        ISteamNetworkingSockets *steam;
        HSteamListenSocket server = 0;
        HSteamNetConnection connection = 0;

        void Init();
    };

}