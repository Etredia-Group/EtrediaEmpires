module UI;
import Engine;
import utils;

ee_engine::UI::UI(const Context &context) :
    renderInterface(context.renderer),
    systemInterface(context.input),
    rmlContext(MakeContext(Rml::Vector2i(context.width, context.height))),
    globalInfo(*rmlContext) {

    globalInfo.data.lang = Engine::localizer->getLocale().code;
    localizerSubscription = Engine::localizer->Subscribe([this](const Locale &locale) {
        globalInfo.data.lang = locale.code;
        globalInfo.DirtyGlobal("lang");
    });

    Rml::LoadFontFace("assets/fonts/CascadiaCode/CascadiaCodePL-Regular.otf");

    Rml::RegisterBase(
        *rmlContext,
        std::bind_front(
            static_cast<std::string_view (Localizer::*)(std::string_view, std::string_view)>(&Localizer::Localize),
            Engine::localizer
        )
    );

    RegisterController<view::LoadingInfo>("fullscreen/loadscreen");
    RegisterController<controller::MainMenu>("fullscreen/mainmenu");
    RegisterController<view::RegionInfo>("popups/region-info");
}

ee_engine::UI::~UI() {
    Engine::localizer->Unsubscribe(localizerSubscription);

    for (UIBaseController *controller : controllers) {
        if (controller != nullptr)
            Deallocate(controller);
    }

    Rml::Shutdown();
}

void ee_engine::UI::Update() const {
    rmlContext->Update();
}

void ee_engine::UI::SwitchScene(const std::string &asset) {
    Rml::ElementDocument *scene = FindOrLoadDocument(asset);

    if (!scene) {
        LOG(ERROR, "Failed to load scene [%hs]", asset.c_str());
        return;
    }

    if (currentDocument.document) {
        currentDocument.document->Hide();
    }

    currentDocument.document = scene;
    currentDocument.asset = asset;

    currentDocument.document->Show();

    LOG(INFO, "Scene [%hs] loaded.", asset.c_str());
}

Rml::ElementDocument* ee_engine::UI::Popup(const std::string &asset, const bool isDraggable) {
    Rml::ElementDocument *popup = FindOrLoadDocument(asset);

    if (!popup) {
        LOG(ERROR, "Failed to load popup [%hs]", asset.c_str());
        return nullptr;
    }

    popup->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    return popup;
}

std::size_t ee_engine::UI::ControllerId(const std::string_view key) {
    static std::unordered_map<std::string, std::size_t, ee_utils::StringHash, ee_utils::StringEqual> ids;
    return ids.try_emplace(std::string(key), ids.size()).first->second;
}

Rml::Context* ee_engine::UI::MakeContext(const Rml::Vector2i dimentions) const {
    Rml::SetSystemInterface(&systemInterface);
    Rml::SetRenderInterface(&renderInterface);

    Rml::Initialise();
    return Rml::CreateContext("main", dimentions);
}

Rml::ElementDocument* ee_engine::UI::FindOrLoadDocument(const std::string &asset) {
    Rml::ElementDocument *document;

    if (const auto cached = documents.find(asset); cached != documents.end()) {
        document = cached->second;
    }
    else {
        std::filesystem::path path = asset;
        path = "assets/layout"/path/path.filename().replace_extension("rml");
        document = rmlContext->LoadDocument(path.string());
        documents[asset] = document;
    }

    return document;
}