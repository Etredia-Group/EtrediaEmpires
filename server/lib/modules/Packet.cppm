module;

//#include <ee_utils/serialization.hpp>

export module Network:Packet;
import std;

export namespace ee_network {

    struct PacketBase {
        std::span<std::byte> payload;
    };

    template <typename T>
    struct Packet : PacketBase {
        explicit Packet(const T &data) {

        }

        ~Packet() {

        }
    };

}