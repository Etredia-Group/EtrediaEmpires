export module AssetData;
export import GameData;
export import Economy;
export import views;
import std;

export namespace ee_assets {
    struct AssetTag {};

    template <typename T> concept Asset = requires { typename T::asset_tag; };

    enum AssetType {
        BINARY,

        MAP_MESH,
        SAVE,
        RESOURCE_LIST,
        ENTERPRISE_LIST,

        SHADER,
        MATERIAL,
        IMAGE
    };

    struct AssetBinary {
        using asset_tag = AssetTag;

        std::uint8_t *content = nullptr;
        std::size_t size = 0;

        AssetBinary(std::uint8_t * & source, std::size_t sourceSize);
        ~AssetBinary();
    };

    struct AssetMapMesh {
        using asset_tag = AssetTag;

        ee_engine::MapMesh content;
    };

    struct AssetSave {
        using asset_tag = AssetTag;

        ee_data::GameState content;
    };

    struct AssetResourceList {
        using asset_tag = AssetTag;

        ee_economy::ResourceList content;
    };

    struct AssetEnterpriseList {
        using asset_tag = AssetTag;

        ee_economy::EnterpriseList content;
    };

    using AssetShader = AssetBinary;

    struct AssetMaterial {
        using asset_tag = AssetTag;

        std::uint32_t vertexShaderAssetID;
        std::uint32_t fragmentShaderAssetID;
    };
}
