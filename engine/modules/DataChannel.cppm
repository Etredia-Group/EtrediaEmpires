module;

export module DataChannel;

export
{
    class IDataChannel
    {
    public:
        // virtual void Request() = 0;
        // virtual void Listen() = 0;
        // virtual void Mute() = 0;
    };

    class NetworkChannel final : public IDataChannel
    {

    };

    class ThreadChannel final : public IDataChannel
    {

    };
}