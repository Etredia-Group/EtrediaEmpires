module;

#include <cstdint>
#include <cstddef>

export module buffers;

export namespace ee_utils
{
    bool readNext(void* dst, std::uint8_t* &srcPointer, std::size_t &srcLeftoverSize, const std::size_t &readSize);
    bool writeNext(std::uint8_t* &dstPointer, void* src, std::size_t &dstLeftoverSize, const std::size_t &writeSize);
}