#pragma once
#include <cstdint>

namespace ee_utils
{
    bool readNext(void* dist, uint8_t* &srcPointer, size_t &srcLeftoverSize, const size_t &readSize);
}