module;

#include <cstdint>
#include <cstddef>
#include <cstring>

module utils;
    import :buffers;

bool ee_utils::readNext(void* dst, std::uint8_t* &srcPointer, std::size_t &srcLeftoverSize, const std::size_t readSize)
{
    if (srcLeftoverSize >= readSize)
    {
        srcLeftoverSize -= readSize;
        std::memcpy(dst, srcPointer, readSize);
        srcPointer += readSize;

        return true;
    }

    return false;
}

bool ee_utils::writeNext(std::uint8_t* &dstPointer, void* src, std::size_t &dstLeftoverSize, const std::size_t writeSize)
{
    if (dstLeftoverSize >= writeSize)
    {
        dstLeftoverSize -= writeSize;
        std::memcpy(dstPointer, src, writeSize);
        dstPointer += writeSize;

        return true;
    }

    return false;
}