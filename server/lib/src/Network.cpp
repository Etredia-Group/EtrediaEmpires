module;

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#define BIND_STEAM_CALLBACK(type, method) reinterpret_cast<void*>(&type::method)
#define GET_LOBBY_SERVER(callbackEvent) LobbyServer *self = \
    reinterpret_cast<LobbyServer*>(callbackEvent->m_info.m_nUserData)
#define GET_LOBBY_CLIENT(callbackEvent) LobbyClient *self = \
    reinterpret_cast<LobbyClient*>(callbackEvent->m_info.m_nUserData)

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
    GameNetworkingSockets_Kill();
}

void ee_network::Lobby::Tick() {
    Poll();
    steam->RunCallbacks();
}

void ee_network::Lobby::Poll() {
    ISteamNetworkingMessage *messages[MAX_MESSAGES_ON_POLL];
    const std::int32_t messagesCount = ReceiveMessages(messages);
    for (std::int32_t i = 0; i < messagesCount; i++) {
        // TODO: handle message
        messages[i]->Release(); // Can be released in ECS dependent on handling realization
    }
}

bool ee_network::Lobby::ASSERT_CONNECTED(std::source_location location) const {
    if (!isConnected) {
        LOG(WARNING, "Failed: No connection.", location);
    }
    return isConnected;
}

ee_network::LobbyServer::~LobbyServer() {
    if (isConnected) {
        for (const HSteamNetConnection &client : clients | std::views::keys) {
            steam->CloseConnection(
                client,
                DISCONNECTION_STATUS_SERVER_CLOSED,
                "Disconnected due server is closed",
                false
            );
        }

        steam->CloseListenSocket(server);
        steam->DestroyPollGroup(pollGroup);

        LOG(INFO, "Server is closed.");
    }
}

const std::int32_t ee_network::LobbyServer::ReceiveMessages(ISteamNetworkingMessage* *receivedMessages) {
    return steam->ReceiveMessagesOnPollGroup(pollGroup, receivedMessages, MAX_MESSAGES_ON_POLL);
}

void ee_network::LobbyServer::MakeConfig() {
    config
        .Set(
            k_ESteamNetworkingConfig_ConnectionUserData,
            reinterpret_cast<std::int64_t>(this)
        )
        .Set(
            k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
            BIND_STEAM_CALLBACK(LobbyServer, OnStatusChanged)
        );
}

void ee_network::LobbyServer::OnStatusChanged(const SteamNetConnectionStatusChangedCallback_t *event) {
    GET_LOBBY_SERVER(event);

    switch (event->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_Connecting:
        {
            if (
                const EResult result = self->steam->AcceptConnection(event->m_hConn);
                result != k_EResultOK
            ) {
                LOG(WARNING, "Failed to accept client connection from [%hs]. Code: [%d].",
                    AddressToString(event->m_info.m_addrRemote).c_str(), result);

                self->steam->CloseConnection(
                    event->m_hConn,
                    DISCONNECTION_STATUS_FAILED_TO_ACCEPT,
                    "Server failed to accept connection",
                    false
                );
            }

            self->steam->SetConnectionPollGroup(event->m_hConn, self->pollGroup);
            LOG(INFO, "Connecting to [%hs].", AddressToString(event->m_info.m_addrRemote).c_str());
            break;
        }

        case k_ESteamNetworkingConnectionState_Connected:
        {
            std::string address = AddressToString(event->m_info.m_addrRemote);
            self->clients.emplace(event->m_hConn, address);
            LOG(INFO, "Connected to [%hs].", address.c_str());
            break;
        }

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        {
            self->clients.erase(event->m_hConn);
            self->steam->CloseConnection(
                event->m_hConn,
                DISCONNECTION_STATUS_FREE_DEAD_CONNECTION_LOCALLY,
                nullptr,
                false
            );
            LOG(INFO, "Free closed connection [%hs].", self->clients[event->m_hConn].c_str());
            break;
        }

        default: break;
    }
}

ee_network::LobbyClient::~LobbyClient() {
    if (isConnected) {
        steam->CloseConnection(
            connection,
            DISCONNECTION_STATUS_NETWORKING_DESTROY,
            "Disconnected due app networking close",
            false
        );
    }
}

void ee_network::LobbyServer::Create(const std::uint16_t port) {
    SteamNetworkingIPAddr address; address.Clear();
    address.m_port = port;

    MakeConfig();
    server = steam->CreateListenSocketIP(
        address,
        static_cast<std::int32_t>(config.options.size()),
        config.options.data()
    );
    pollGroup = steam->CreatePollGroup();

    isConnected = true;
    LOG(INFO,"Server started: [%hs].", AddressToString(address).c_str());
}

void ee_network::LobbyServer::Broadcast(const PacketBase &packet, SendFlags flags) {
    LOG(ERROR, "NOT IMPLEMENTED!"); // TODO: use steam->SendMessages for optimal broadcasting
}

void ee_network::LobbyClient::Connect(const char *addressStr) {
    LOG(INFO, "Connecting to server [%hs].", addressStr);
    SteamNetworkingIPAddr address; address.Clear(); address.ParseString(addressStr);

    MakeConfig();

    connection = steam->ConnectByIPAddress(
        address,
        static_cast<std::int32_t>(config.options.size()),
        config.options.data()
    );

    endpointAddress = addressStr;
}

void ee_network::LobbyClient::Disconnect() {
    steam->CloseConnection(
        connection,
        DISCONNECTION_STATUS_CLIENT_ACTION,
        "Disconnected by client",
        false
    );
    isConnected = false;

    LOG(INFO, "Disconnected from [%hs].", endpointAddress.c_str());
    endpointAddress = "";
    connection = k_HSteamNetConnection_Invalid;
}

void ee_network::LobbyClient::Send(const PacketBase &packet, SendFlags flags) const {
    if (!ASSERT_CONNECTED()) return;

    EResult result = steam->SendMessageToConnection(
        connection,
        packet.payload.data(),
        static_cast<std::int32_t>(packet.payload.size()),
        static_cast<std::int32_t>(flags),
        nullptr
    );

#ifndef NDEBUG
    LOG(INFO, "Send packet to [%hs]. Code: %d.", endpointAddress.c_str(), result);
#endif
}

const std::int32_t ee_network::LobbyClient::ReceiveMessages(ISteamNetworkingMessage* *receivedMessages) {
    return steam->ReceiveMessagesOnConnection(connection, receivedMessages, MAX_MESSAGES_ON_POLL);
}

void ee_network::LobbyClient::MakeConfig() {
    config
        .Set(
            k_ESteamNetworkingConfig_ConnectionUserData,
            reinterpret_cast<std::int64_t>(this)
        )
        .Set(
            k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
            BIND_STEAM_CALLBACK(LobbyClient, OnStatusChanged)
        );
}

void ee_network::LobbyClient::OnStatusChanged(const SteamNetConnectionStatusChangedCallback_t *event) {
    GET_LOBBY_CLIENT(event);

    switch (event->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_Connected:
        {
            self->isConnected = true;
            LOG(INFO, "Connected to [%hs].", self->endpointAddress.c_str());
            break;
        }

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        {
            self->isConnected = false;
            self->steam->CloseConnection(
                event->m_hConn,
                DISCONNECTION_STATUS_FREE_DEAD_CONNECTION_LOCALLY,
                nullptr,
                false
            );
            self->connection = k_HSteamNetConnection_Invalid;
            LOG(INFO, "Free closed connection [%hs].", self->endpointAddress.c_str());
            break;
        }

        default: break;
    }
}
