#include "AssetData.hpp"
#include "allocator.hpp"

AssetBinary::AssetBinary(uint8_t* &source, const size_t &sourceSize)
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

AssetShader::AssetShader(uint8_t* &source, const size_t &sourceSize)
{
    content = reinterpret_cast<uint32_t*>(source);
    size = sourceSize;
    source = nullptr;
}

AssetShader::~AssetShader()
{
    if (content != nullptr)
        DeallocateArray(content);
}
