#pragma once
#include "Map.hpp"
#include <concepts>
#include <cstdint>

namespace ee_assets
{
    struct AssetData {};
    template<typename T> concept Asset = std::derived_from<T, AssetData>;
    
    struct AssetBinary : AssetData
    {
        uint8_t* content = nullptr;
        size_t size = 0;
    
        AssetBinary(uint8_t* &source, const size_t &sourceSize);
        ~AssetBinary();
    };
    
    struct AssetMapMesh : AssetData
    {
        ee_engine::MapMesh content;
    
        AssetMapMesh(uint8_t* &source, const size_t &sourceSize);
    };
    
    #define AssetShader AssetBinary
    
    struct AssetMaterial : AssetData
    {
        uint32_t vertexShaderAssetID;
        uint32_t fragmentShaderAssetID;
    };
}