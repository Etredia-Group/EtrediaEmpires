#pragma once

#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace ee_engine
{
    struct MapMesh
    {
        struct Province
        {
            std::vector<glm::vec2> vertices;
            std::vector<uint16_t> indices;
        };
        std::unordered_map<uint32_t, std::vector<Province>> states;
    };
    
    struct ProvinceSSBO
    {
        uint32_t stateId;
        uint32_t color;
    };
}