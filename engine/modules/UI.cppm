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
        Rml::Binded<ee_engine::RegionInfo> regionInfo;

        UI(Rml::RenderInterface &renderer, Rml::SystemInterface &input, std::uint32_t width, std::uint32_t height);
        ~UI();

        void Update() const;

        Rml::ElementDocument* Popup(const std::string &asset, bool isDraaggable=true);

        constexpr Rml::Context* getContext() const noexcept {return context;}

    private:
        Rml::ElementDocument *currentDocument;
        std::unordered_map<std::string, Rml::ElementDocument*> documents;
        Rml::Element *popupsContatiner;

        Rml::Context* MakeContext(Rml::Vector2i dimentions) const;
        void LoadPopups();
    };
}
