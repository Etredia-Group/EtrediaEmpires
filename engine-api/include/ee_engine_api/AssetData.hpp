#pragma once
#include "GameData.hpp"
#include "Map.hpp"
#include <concepts>
#include <cstdint>
#include <vector>

namespace ee_assets
{
    struct AssetData {};
    template<typename T> concept Asset = std::derived_from<T, AssetData>;

    enum AssetType
    {
        BINARY,

        MAP_MESH,
        SAVE,

        SHADER,
        MATERIAL,
        IMAGE
    };
    
    struct AssetBinary : public AssetData
    {
        uint8_t* content = nullptr;
        size_t size = 0;
    
        AssetBinary(uint8_t* &source, const size_t sourceSize);
        ~AssetBinary();
    };
    
    struct AssetMapMesh : public AssetData
    {
        ee_engine::MapMesh content;
    
        uint8_t* Serialize(size_t &size);
        AssetMapMesh() {};
        AssetMapMesh(uint8_t* &source, const size_t sourceSize);
    };

    struct AssetSave : public AssetData
    {
        std::vector<ee_data::Subject> subjects;

        uint8_t* Serialize(size_t &size);
        AssetSave() {};
        AssetSave(uint8_t* &source, const size_t sourceSize);
    };
    
    #define AssetShader AssetBinary
    
    struct AssetMaterial : public AssetData
    {
        uint32_t vertexShaderAssetID;
        uint32_t fragmentShaderAssetID;
    };
}