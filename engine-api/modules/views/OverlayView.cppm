export module views:OverlayView;
import std;

export namespace ee_engine {

    struct LoadingInfo {
        float progress = 0.0f;
        std::string portrait = "";
        std::string stage = "";
        std::string quote = "";
    };

    struct RegionInfo {
        std::uint32_t id = 0;
        std::string name = "";
    };

}