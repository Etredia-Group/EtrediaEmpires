module;

#include <steam/isteamnetworkingsockets.h>

export module Network;
import :Packet;
import SteamExtention;
import std;

export namespace ee_network {
    using EndpointAddress = std::string;

    class Lobby {
    public:
        explicit Lobby();
        virtual ~Lobby();

        void Tick();

    protected:
        static constexpr std::int32_t MAX_MESSAGES_ON_POLL = 64;
        bool isConnected = false;

        ISteamNetworkingSockets *steam;
        SteamNetConfig config;

        void Poll();
        virtual const std::int32_t ReceiveMessages(ISteamNetworkingMessage* *receivedMessages) = 0;

        [[nodiscard]] bool ASSERT_CONNECTED(std::source_location location=std::source_location::current()) const;
    };

    class LobbyServer final : public Lobby {
    public:
        ~LobbyServer() override;
        void Create(std::uint16_t port);

        void Broadcast(const PacketBase &packet, SendFlags flags);

    protected:
        const std::int32_t ReceiveMessages(ISteamNetworkingMessage **receivedMessages) override;

    private:
        HSteamListenSocket server = k_HSteamListenSocket_Invalid;
        HSteamNetPollGroup pollGroup = k_HSteamNetPollGroup_Invalid;
        std::unordered_map<HSteamNetConnection, EndpointAddress> clients;

        void MakeConfig();

        static void OnStatusChanged(const SteamNetConnectionStatusChangedCallback_t *event);
    };

    class LobbyClient final : public Lobby {
    public:
        ~LobbyClient() override;
        void Connect(const char *addressStr);
        void Disconnect();

        void Send(const PacketBase &packet, SendFlags flags) const;

    protected:
        const std::int32_t ReceiveMessages(ISteamNetworkingMessage **receivedMessages) override;

    private:
        HSteamNetConnection connection = k_HSteamNetConnection_Invalid;
        EndpointAddress endpointAddress = "";

        void MakeConfig();

        static void OnStatusChanged(const SteamNetConnectionStatusChangedCallback_t *event);
    };

}