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
    regionInfo(*context) {

    Rml::LoadFontFace("assets/fonts/ARIAL.TTF");

    currentDocument = context->LoadDocument("assets/layout/overlay/overlay.rml");
    popupsContatiner = currentDocument->GetElementById("popups-container");
    if (!popupsContatiner) {
        LOG(ERROR, "Layout does not contain popups-container element.");
    }
    LoadPopups();

    currentDocument->Show();
}

UI::~UI() {
    Rml::Shutdown();
}

void UI::Update() const {
    context->Update();
}

Rml::ElementDocument* UI::Popup(const std::string &asset, const bool isDraaggable) {
    Rml::ElementDocument *popup;

    if (const auto cached = documents.find(asset); cached != documents.end()) {
        popup = cached->second;
    }
    else {
        popup = context->LoadDocument(asset);
        documents[asset] = popup;
    }

    if (!popup) {
        LOG(ERROR, "Failed to load popup [%s]", asset);
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

void UI::LoadPopups() {
    const std::vector<std::string> popupAssets {
        "region-info"
    };

    for (std::string popupAsset : popupAssets) {
        std::string path = "assets/layout/popups/" + popupAsset + "/" + popupAsset + ".rcss";
        documents[path] = context->LoadDocument(path);
    }
}
