module Client;
import utils;
import views;
import Engine;

ee_engine::Client::Client() : ThreadedWorker(L"Client", 0) {
    commandsMapping[ClientCommand::SEND] = std::bind_front(&Client::SendCmd, this);
}

std::generator<float> ee_engine::Client::Init() const {
    if (Engine::renderer == nullptr || !Engine::renderer->isReady()) {
        LOG(CRITICAL, "Renderer is not ready. Client cannot be created");
        throw std::logic_error("Renderer is not ready");
    }

    // TEST
    ee_assets::Save& asset = Engine::assetManager->Get<ee_assets::Save>(save);
    float saveLoadProgress = 0.0f;
    for (const ee_data::Subject &subject : asset.save.subjects) {
        ViewModelSubject(subject);
        saveLoadProgress++;
        co_yield saveLoadProgress / asset.save.subjects.size();
    }
}

ee_engine::IDataChannel* ee_engine::Client::GetConnection() const {
    return networkConnection;
}

void ee_engine::Client::ConnectServer(const char *ip) {
    if (networkConnection != nullptr)
        Deallocate(networkConnection);

    networkConnection = Allocate<NetworkChannel>();
    networkConnection->lobby.Connect(ip);
}

void ee_engine::Client::LoadSave(const AssetHandle save) {
    this->save = save;
}

void ee_engine::Client::Update() {
    if (networkConnection != nullptr) {
        networkConnection->lobby.Tick();
    }
}

void ee_engine::Client::Release() {}


// ReSharper disable once CppMemberFunctionMayBeStatic
void ee_engine::Client::SendCmd(ClientCommand& command) {
    command.type = ClientCommand::COMPLETED;
}


void ee_engine::Client::ViewModelSubject(const ee_data::Subject &subject) {
    for (const std::uint32_t region : subject.regions) {
        const ProvinceSSBO vm {
            .regionId = region,
            .color = subject.color
        };

        RendererCommand cmd;
        cmd.type = RendererCommand::UPDATE_REGION;
        cmd.region = {.id = region, .data = vm};

        Engine::renderer->Request(cmd);
    }
}


ee_engine::Client::~Client() {
    if (networkConnection != nullptr)
        Deallocate(networkConnection);
}
