module;

export module DataChannel;
import std;

export
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
        void Request() override;
    };

    class ThreadChannel final : public IDataChannel {
    public:
        void Request() override;
    };
}
