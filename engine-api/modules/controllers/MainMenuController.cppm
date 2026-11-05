export module controllers:MainMenuController;
import :UIController;
import views;
import utils;
import std;

export namespace ee_engine::controller {

    class MainMenu final : public UIController<view::MainMenu> {
    public:
        MainMenu(Rml::Context &rmlContext, Rml::Binded<view::GlobalInfo> &globalInfo);
    protected:
        void OnLoad() override;
    };

}