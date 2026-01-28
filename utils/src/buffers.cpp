#include <ee_utils/buffers.hpp>
#include <cstring>

bool ee_utils::readNext(void* dst, uint8_t* &srcPointer, size_t &srcLeftoverSize, const size_t &readSize)
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

bool ee_utils::writeNext(uint8_t* &dstPointer, void* src, size_t &dstLeftoverSize, const size_t &writeSize)
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