export module AssetData;
import GameData;
import views;
import std;

export namespace ee_assets {
    struct AssetData {};

    template <typename T> concept Asset = std::is_base_of_v<AssetData, T>;

    enum AssetType {
        BINARY,

        MAP_MESH,
        SAVE,

        SHADER,
        MATERIAL,
        IMAGE
    };

    struct AssetBinary : public AssetData {
        std::uint8_t *content = nullptr;
        std::size_t size = 0;

        AssetBinary(std::uint8_t * & source, const std::size_t sourceSize);
        ~AssetBinary();
    };

    struct AssetMapMesh : public AssetData {
        ee_engine::MapMesh content;

        std::uint8_t* Serialize(std::size_t& size);
        AssetMapMesh() {};
        AssetMapMesh(std::uint8_t * & source, const std::size_t sourceSize);
    };

    struct AssetSave : public AssetData {
        ee_data::GameState content;

        std::uint8_t* Serialize(std::size_t& size);
        AssetSave() {};
        AssetSave(std::uint8_t * & source, const std::size_t sourceSize);
    };

    using AssetShader = AssetBinary;

    struct AssetMaterial : public AssetData {
        std::uint32_t vertexShaderAssetID;
        std::uint32_t fragmentShaderAssetID;
    };
}
