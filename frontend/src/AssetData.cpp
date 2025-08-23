#include "AssetData.hpp"
#include "allocator.hpp"

AssetBinary::AssetBinary(uint8_t* &source, size_t sourceSize)
{
    content = source;
    size = sourceSize;
    source = nullptr;
}

AssetBinary::~AssetBinary()
{
    if (content != nullptr)
        DeallocateArray(content);
}