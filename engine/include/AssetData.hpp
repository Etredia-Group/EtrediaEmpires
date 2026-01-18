#pragma once
#include <concepts>
#include <cstdint>

struct AssetData {};
template<typename T> concept Asset = std::derived_from<T, AssetData>;

struct AssetBinary : AssetData
{
    uint8_t* content = nullptr;
    size_t size = 0;

    AssetBinary(uint8_t* &source, const size_t &sourceSize);
    ~AssetBinary();
};

struct AssetVulkanShader : AssetData
{
    uint32_t* content = nullptr;
    size_t size = 0;

    AssetVulkanShader(uint8_t* &source, const size_t &sourceSize);
    ~AssetVulkanShader();
};

struct AssetVulkanMaterial : AssetData
{
    uint32_t vertexShaderAssetID;
    uint32_t fragmentShaderAssetID;
    uint32_t renderType;
    uint32_t renderStage;

    // Vertex shader params
    // <param id:value>

    // Fragment shader params
    // <param id:value>
};