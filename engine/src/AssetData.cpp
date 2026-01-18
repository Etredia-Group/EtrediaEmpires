#include "AssetData.hpp"
#include <ee_utils/allocator.hpp>

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

AssetVulkanShader::AssetVulkanShader(uint8_t* &source, const size_t &sourceSize)
{
    content = reinterpret_cast<uint32_t*>(source);
    size = sourceSize;
    source = nullptr;
}

AssetVulkanShader::~AssetVulkanShader()
{
    if (content != nullptr)
        DeallocateArray(content);
}
