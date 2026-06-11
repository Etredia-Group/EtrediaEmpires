export module UI;
import rml;
import std;

export {
    class UI {
    public:
        UI(Rml::RenderInterface &renderer, std::uint32_t width, std::uint32_t height);
        ~UI();

        void Update() const;

        constexpr Rml::Context* getContext() const noexcept {return context;}

    private:
        Rml::RenderInterface &renderInterface;
        Rml::Context *context;

        Rml::ElementDocument *currentDocument;
    };
}