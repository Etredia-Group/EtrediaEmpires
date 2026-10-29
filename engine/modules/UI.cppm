export module UI;
import std;
export import rml;
import views;

export {
    class UI {
    private:
        Rml::RenderInterface &renderInterface;
        Rml::SystemInterface &systemInterface;
        Rml::Context *context;

    public:
        struct Scene {
            std::string asset;
            Rml::ElementDocument *document = nullptr;
        };

        Rml::Binded<ee_engine::LoadingInfo> loadingInfo;
        Rml::Binded<ee_engine::RegionInfo> regionInfo;

        UI(Rml::RenderInterface &renderer, Rml::SystemInterface &input, std::uint32_t width, std::uint32_t height);
        ~UI();

        void Update() const;

        void SwitchScene(const std::string &asset);
        Rml::ElementDocument* Popup(const std::string &asset, bool isDraggable=true);

        constexpr Rml::Context* getContext() const noexcept {return context;}
        constexpr Scene getScene() const noexcept {return currentDocument;}

    private:
        Scene currentDocument;
        std::unordered_map<std::string, Rml::ElementDocument*> documents;
        Rml::Element *popupsContatiner;

        Rml::Context* MakeContext(Rml::Vector2i dimentions) const;
        Rml::ElementDocument* FindOrLoadDocument(const std::string &asset);
        void LoadPopups();
    };
}
