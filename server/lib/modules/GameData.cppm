module;

#include <cstdint>
#include <vector>

export module GameData;

export namespace ee_data
{
    struct Subject
    {
        std::uint32_t tag;
        std::uint32_t color;
        std::vector<std::uint32_t> regions;
    };

    struct GameState
    {
        std::vector<Subject> subjects;
    };
}