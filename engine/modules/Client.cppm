export module Client;
import utils;
import GameData;
import DataChannel;
import AssetManager;
import std;

export
{
    struct ClientCommand : public ee_utils::WorkerCommand
    {
        enum Type : std::uint8_t
        {
            NONE,

            SEND,

            COMPLETED
        };
    };

    class Client final : public ee_utils::ThreadedWorker<ClientCommand, ClientCommand::COMPLETED>
    {
    public:
        Client();
        ~Client();
        void Init();

        //void Send(); -> Request(SEND)

        const IDataChannel* GetConnection();
        void ConnectServer();

        void LoadSave(const AssetHandle save);

    protected:
        void Update() override;
        void Release() override;

    private:
        NetworkChannel *networkConnection = nullptr;
        AssetHandle save;

        void SendCmd(ClientCommand &command);

        void ViewModelSubject(ee_data::Subject subject);
    };
}