#include <ee_utils/buffers.hpp>
#include <cstring>

bool readNext(void* dist, uint8_t* &srcPointer, size_t &srcLeftoverSize, const size_t &readSize)
{
    if (srcLeftoverSize >= readSize)
    {
        srcLeftoverSize -= readSize;
        std::memcpy(dist, srcPointer, readSize);
        srcPointer += readSize;

        return true;
    }

    return false;
}