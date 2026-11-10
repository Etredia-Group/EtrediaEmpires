module;

export module DataChannel;
import Network;
import std;

export namespace ee_engine
{
    class IDataChannel {
    public:
        virtual ~IDataChannel() = default;

        virtual void Request() = 0;
        // virtual void Listen() = 0;
        // virtual void Mute() = 0;
    };

    class NetworkChannel final : public IDataChannel {
    public:
        ee_network::LobbyClient lobby;

        void Request() override;
    };

    // class ThreadChannel final : public IDataChannel {
    // public:
    //     void Request() override;
    // };
}
