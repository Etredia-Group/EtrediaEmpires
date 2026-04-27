export module Packets;
import std;

export namespace ee_data {
    enum class Route {
        LOAD_SAVE
    };

    struct Request {
        Route route;
    };

    struct Response {
        Route route;
        std::span<std::byte> payload;
    };
}