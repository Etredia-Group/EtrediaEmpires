module;

#include <steam/isteamnetworkingsockets.h>

export module Network;
import SteamExtention;
import std;

export namespace ee_network {

    class Lobby {
    public:
        explicit Lobby();
        virtual ~Lobby();

        void Tick();

    protected:
        const std::int32_t MAX_MESSAGES_ON_POLL = 64;
        bool isConnected = false;

        ISteamNetworkingSockets *steam;

        void Poll();
        virtual const std::int32_t ReceiveMessages(ISteamNetworkingMessage* *receivedMessages) = 0;
    };

    class LobbyServer final : public Lobby {
    public:
        ~LobbyServer() override;
        void Create(std::uint16_t port);

    protected:
        const std::int32_t ReceiveMessages(ISteamNetworkingMessage **receivedMessages) override;

    private:
        HSteamListenSocket server = 0;
        HSteamNetPollGroup pollGroup = 0;
        std::set<HSteamNetConnection> clients;
        SteamNetConfig config;

        void MakeConfig();

        static void OnStatusChanged(const SteamNetConnectionStatusChangedCallback_t *event);
    };

    class LobbyClient final : public Lobby {
    public:
        ~LobbyClient() override;
        void Connect(const char *addressStr);
        void Disconnect();

    protected:
        const std::int32_t ReceiveMessages(ISteamNetworkingMessage **receivedMessages) override;

    private:
        HSteamNetConnection connection = 0;
    };

}