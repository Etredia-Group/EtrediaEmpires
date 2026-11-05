module controllers;
import Engine;

ee_engine::controller::MainMenu::MainMenu(Rml::Context &rmlContext, Rml::Binded<view::GlobalInfo> &globalInfo) :
    UIController(rmlContext, globalInfo) {}

void ee_engine::controller::MainMenu::OnLoad() {
    BindEvent("connect-ip", "click", [this](Rml::Event&) {
        const std::string &ip = data.data.ip;
        Engine::client->ConnectServer(ip.c_str());
    });
}
