module Client;
import utils;
import views;
import Engine;

Client::Client() : ee_utils::ThreadedWorker<ClientCommand, ClientCommand::COMPLETED>(L"Client")
{
    commandsMapping[ClientCommand::SEND] = std::bind_front(&Client::SendCmd, this);
}

void Client::Init()
{
    if (Engine::renderer == nullptr || !Engine::renderer->isReady())
    {
        LOG(CRITICAL, "Renderer is not ready. Client cannot be created");
        throw std::logic_error("Renderer is not ready");
    }

    // TEST
    ee_assets::AssetSave &asset = Engine::assetManager->Get<ee_assets::AssetSave>(save);
    ViewModelSubject(asset.content.subjects[0]);
    ViewModelSubject(asset.content.subjects[1]);
}

const IDataChannel* Client::GetConnection()
{
    return networkConnection;
}

void Client::ConnectServer()
{
    if (networkConnection != nullptr)
        Deallocate(networkConnection);

    networkConnection = Allocate<NetworkChannel>();
}

void Client::LoadSave(const AssetHandle save)
{
    this->save = save;
}

void Client::Update()
{

}

void Client::Release()
{

}


void Client::SendCmd(ClientCommand &command)
{


    command.type = ClientCommand::COMPLETED;
}


void Client::ViewModelSubject(ee_data::Subject subject)
{
    for (const std::uint32_t region : subject.regions)
    {
        ee_engine::ProvinceSSBO vm {
            .regionId = region,
            .color = subject.color
        };

        ee_engine::RendererCommand cmd;
        cmd.type = ee_engine::RendererCommand::UPDATE_REGION;
        cmd.region = {.id = region, .data = vm};

        Engine::renderer->Request(cmd);
    }
}



Client::~Client()
{
    if (networkConnection != nullptr)
        Deallocate(networkConnection);
}