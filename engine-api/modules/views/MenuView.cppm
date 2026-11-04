export module views:MenuView;
import std;

export namespace ee_engine::view {

    struct LoadingInfo {
        using view_tag = void;

        float progress = 0.0f;
        std::string portrait = "";
        std::string stage = "";
        std::string quote = "";
    };

    struct MainMenu {
        using view_tag = void;

        std::string ip = "";
    };

}