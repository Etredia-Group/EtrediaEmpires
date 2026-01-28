#pragma once
#include <cstdint>
#include <vector>

namespace ee_data
{
    struct Subject
    {
        uint32_t tag;
        uint32_t color;
        std::vector<uint32_t> regions;
    };
}