export module controllers:MainMenuController;
import :UIController;
import views;
import utils;
import std;

export namespace ee_engine::controller {

    class MainMenu final : public UIController<view::MainMenu> {
    public:
        MainMenu(Rml::Context &rmlContext, Rml::Binded<view::GlobalInfo> &globalInfo)
            : UIController(rmlContext, globalInfo) {}

    protected:
        void OnLoad() override {
            BindEvent("connect-ip", "click", [](Rml::Event &event) {
                LOG(INFO, "Connecting to IP: [%hs]", "...");
            });
        }
    };

}