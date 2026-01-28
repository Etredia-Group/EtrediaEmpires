#pragma once
#include <cstdint>

#define as_bytes(value) reinterpret_cast<const char*>(value)

namespace ee_utils
{
    bool readNext(void* dst, uint8_t* &srcPointer, size_t &srcLeftoverSize, const size_t &readSize);
    bool writeNext(uint8_t* &dstPointer, void* src, size_t &dstLeftoverSize, const size_t &writeSize);
}