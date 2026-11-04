export module views:OverlayView;
import std;

export namespace ee_engine::view {

    struct RegionInfo {
        using view_tag = void;

        std::uint32_t id = 0;
        std::string name = "";
    };

}