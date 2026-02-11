module;

#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cstdint>

export module views:MapView;

export namespace ee_engine
{
    struct MapMesh
    {
        struct Province
        {
            std::vector<glm::vec2> vertices;
            std::vector<std::uint16_t> indices;
        };
        std::unordered_map<std::uint32_t, std::vector<Province>> regions;
    };

    struct ProvinceSSBO
    {
        std::uint32_t regionId;
        std::uint32_t color;
    };
}