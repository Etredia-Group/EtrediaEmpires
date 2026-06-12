module UI;

UI::UI(Rml::RenderInterface &renderer, const std::uint32_t width, const std::uint32_t height) :
    renderInterface(renderer) {
    Rml::SetRenderInterface(&renderInterface);

    Rml::Initialise();
    context = Rml::CreateContext("main", Rml::Vector2i(width, height));

    Rml::LoadFontFace("assets/fonts/ARIAL.TTF");

    currentDocument = context->LoadDocument("assets/layout/overlay/overlay.rml");
    currentDocument->Show();
}

UI::~UI() {
    Rml::Shutdown();
}

void UI::Update() const {
    context->Update();
}
