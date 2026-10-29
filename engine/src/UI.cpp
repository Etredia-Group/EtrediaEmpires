module UI;
import utils;

UI::UI(
    Rml::RenderInterface &renderer,
    Rml::SystemInterface &input,
    const std::uint32_t width,
    const std::uint32_t height
) :
    renderInterface(renderer),
    systemInterface(input),
    context(MakeContext(Rml::Vector2i(width, height))),
    loadingInfo(*context),
    regionInfo(*context) {

    Rml::LoadFontFace("assets/fonts/ARIAL.TTF");

    LoadPopups();
}

UI::~UI() {
    Rml::Shutdown();
}

void UI::Update() const {
    context->Update();
}

void UI::SwitchScene(const std::string &asset) {
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
    popupsContatiner = currentDocument.document->GetElementById("popups-container");
    if (!popupsContatiner) {
        LOG(ERROR, "Scene [%hs] does not contain popups-container element.", asset.c_str());
    }

    currentDocument.document->Show();

    LOG(INFO, "Scene [%hs] loaded.", asset.c_str());
}

Rml::ElementDocument* UI::Popup(const std::string &asset, const bool isDraggable) {
    Rml::ElementDocument *popup = FindOrLoadDocument(asset);

    if (!popup) {
        LOG(ERROR, "Failed to load popup [%hs]", asset.c_str());
        return nullptr;
    }

    popup->Show(Rml::ModalFlag::None, Rml::FocusFlag::None);
    return popup;
}

Rml::Context* UI::MakeContext(const Rml::Vector2i dimentions) const {
    Rml::SetSystemInterface(&systemInterface);
    Rml::SetRenderInterface(&renderInterface);

    Rml::Initialise();
    return Rml::CreateContext("main", dimentions);
}

Rml::ElementDocument* UI::FindOrLoadDocument(const std::string &asset) {
    Rml::ElementDocument *document;

    if (const auto cached = documents.find(asset); cached != documents.end()) {
        document = cached->second;
    }
    else {
        document = context->LoadDocument(asset);
        documents[asset] = document;
    }

    return document;
}

void UI::LoadPopups() {
    const std::vector<std::string> popupAssets {
        "region-info"
    };

    for (std::string popupAsset : popupAssets) {
        std::string path = "assets/layout/popups/" + popupAsset + "/" + popupAsset + ".rcss";
        documents[path] = context->LoadDocument(path);
    }
}
