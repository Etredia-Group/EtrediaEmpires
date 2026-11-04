export module views;
export import :MapView;
export import :OverlayView;
export import :MenuView;

import std;

export namespace ee_engine {

    template <typename T> concept IView = requires { typename T::view_tag; };

}

export namespace ee_engine::view {

    struct GlobalInfo {
        using view_tag = void;

        std::string lang;
    };

}