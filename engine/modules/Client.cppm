export module Client;
import utils;
import GameData;
import DataChannel;
import AssetManager;
import std;

export
{
    struct ClientCommand : ee_utils::WorkerCommand {
        enum Type : std::uint8_t {
            NONE,

            SEND,

            COMPLETED
        };
    };

    class Client final : public ee_utils::ThreadedWorker<ClientCommand, ClientCommand::COMPLETED> {
    public:
        Client();
        ~Client() override;
        void Init() const;

        //void Send(); -> Request(SEND)

        const IDataChannel* GetConnection() const;
        void ConnectServer();

        void LoadSave(AssetHandle save);

    protected:
        void Update() override;
        void Release() override;

    private:
        NetworkChannel *networkConnection = nullptr;
        AssetHandle save;

        void SendCmd(ClientCommand& command);

        static void ViewModelSubject(const ee_data::Subject &subject);
    };
}
