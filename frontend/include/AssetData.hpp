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

struct AssetShader : AssetData
{
    uint32_t* content = nullptr;
    size_t size = 0;

    AssetShader(uint8_t* &source, const size_t &sourceSize);
    ~AssetShader();
};

struct AssetMaterial : AssetData
{
    uint32_t vertexShaderAssetID;
    uint32_t fragmentShaderAssetID;
    uint32_t renderType;
    uint32_t renderStage;
};