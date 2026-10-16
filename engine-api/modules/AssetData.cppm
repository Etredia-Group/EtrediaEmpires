/*
 * To define new asset:
 * - Define struct with "using asset_tag = AssetTag".
 * - Add template specialization in engine/AssetManager.cpp registry TODO: Move registry to engine-api
 * - Assets loads in Cap'n Proto format by default.
 * - If asset needs custom construction logic from raw binary:
 * - | define and implement constructor (std::uint8_t * & source, std::size_t sourceSize)
 * - | in AssetData.cpp
 */

export module AssetData;
export import GameData;
export import Economy;
export import views;
import std;

export namespace ee_assets {
    struct AssetTag {};

    template <typename T> concept Asset = requires { typename T::asset_tag; };

    struct Binary {
        using asset_tag = AssetTag;

        std::uint8_t *content = nullptr;
        std::size_t size = 0;

        Binary(std::uint8_t * & source, std::size_t sourceSize);
        ~Binary();
    };

    struct MapMesh {
        using asset_tag = AssetTag;

        ee_engine::MapMesh content;
    };

    struct Save {
        using asset_tag = AssetTag;

        ee_data::GameState content;
    };

    struct ResourceList {
        using asset_tag = AssetTag;

        ee_economy::ResourceList content;
    };

    struct EnterpriseList {
        using asset_tag = AssetTag;

        ee_economy::EnterpriseList content;
    };

    using Shader = Binary;

    struct Material {
        using asset_tag = AssetTag;

        std::uint32_t vertexShaderAssetID;
        std::uint32_t fragmentShaderAssetID;
    };
}
