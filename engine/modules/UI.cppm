export module UI;
import std;
export import rml;
export import controllers;
export import views;
import Localizer;
import utils;

export {
    class UI {
        Rml::RenderInterface &renderInterface;
        Rml::SystemInterface &systemInterface;
        Rml::Context *rmlContext;
        Localizer &localizer;

    public:
        struct Context {
            Rml::RenderInterface &renderer;
            Rml::SystemInterface &input;
            Localizer &localizer;
            std::uint32_t width;
            std::uint32_t height;
        };

        struct Scene {
            std::string asset;
            Rml::ElementDocument *document = nullptr;
        };

        explicit UI(const Context &context);
        ~UI();

        void Update() const;

        template <ee_engine::IUIController TUIController> void RegisterController(const std::string &asset);
        template <ee_engine::IView TView> void RegisterController(const std::string &asset="");
        void SwitchScene(const std::string &asset);
        Rml::ElementDocument* Popup(const std::string &asset, bool isDraggable=true);

        Rml::Context* getContext() const noexcept {return rmlContext;}
        const Scene& getScene() const noexcept {return currentDocument;}

        static std::size_t ControllerId(std::string_view key);
        template <ee_engine::IView TView> using Binded = Rml::Binded<TView, Rml::Binded<ee_engine::view::GlobalInfo>>;
        template <ee_engine::IUIController TController> TController& GetController() const;
        template <ee_engine::IView TView> Binded<TView>& GetController() const;

    private:
        Scene currentDocument;
        std::unordered_map<std::string, Rml::ElementDocument*> documents;
        std::uint8_t localizerSubscription = 0;

        Rml::Binded<ee_engine::view::GlobalInfo> globalInfo;
        std::vector<ee_engine::UIBaseController*> controllers;

        Rml::Context* MakeContext(Rml::Vector2i dimentions) const;
        Rml::ElementDocument* FindOrLoadDocument(const std::string &asset);

        template <typename T> static std::size_t ControllerTypeId() noexcept;
        template <typename T> void StoreController(ee_engine::UIBaseController *controller);
    };
}

template <ee_engine::IUIController TUIController>
void UI::RegisterController(const std::string &asset) {
    ee_engine::UIBaseController *controller = Allocate<TUIController>(*rmlContext, globalInfo);
    StoreController<TUIController>(controller);
    controller->Load(FindOrLoadDocument(asset));
}

template <ee_engine::IView TView>
void UI::RegisterController(const std::string &asset) {
    ee_engine::UIBaseController *controller = Allocate<ee_engine::UIController<TView>>(*rmlContext, globalInfo);
    StoreController<TView>(controller);
    controller->Load(FindOrLoadDocument(asset));
}

template <typename T>
std::size_t UI::ControllerTypeId() noexcept {
    static const std::size_t id = ControllerId(typeid(T).name());
    return id;
}

template <typename T>
void UI::StoreController(ee_engine::UIBaseController *controller) {
    const std::size_t id = ControllerTypeId<T>();
    if (controllers.size() <= id) controllers.resize(id + 1, nullptr);

    if (controllers[id]) {
        LOG(WARNING, "UI Controller [%hs] already registered", typeid(T).name());
        Deallocate(controller);
        return;
    }

    controllers[id] = controller;
}

template <ee_engine::IUIController TController>
TController& UI::GetController() const {
    const std::size_t id = ControllerTypeId<TController>();
    return static_cast<TController&>(*controllers[id]);
}

template <ee_engine::IView TView>
UI::Binded<TView>& UI::GetController() const {
    const std::size_t id = ControllerTypeId<TView>();
    return static_cast<ee_engine::UIController<TView>&>(*controllers[id]).getData();
}