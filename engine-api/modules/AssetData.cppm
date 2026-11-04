/*
 * To define new asset:
 * - Define struct with "using asset_tag = AssetTag".
 * - Add template specialization in engine/AssetManager.cpp registry. TODO: Move registry to engine-api
 * - Assets loads in Cap'n Proto format by default.
 * - If asset needs custom construction logic from raw binary:
 * - | define and implement constructor (std::uint8_t * & source, std::size_t sourceSize)
 * - | in AssetData.cpp
 * - Make sure to add asset in SDK if it needs preprocessing.
 */

export module AssetData;
export import GameData;
export import Economy;
export import views;
import utils;
import std;

export namespace ee_assets {
    struct AssetTag {};

    template <typename T> concept Asset = requires { typename T::asset_tag; };

    // Engine assets

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

    using Shader = Binary;

    struct Texture {
        using asset_tag = AssetTag;

        std::uint8_t *content = nullptr;
        std::size_t size = 0;

        std::int32_t dimentionX = 0, dimentionY = 0;
        std::int32_t channels = 0;

        Texture(std::uint8_t * & source, std::size_t sourceSize);
        ~Texture();
    };

    struct Material {
        using asset_tag = AssetTag;

        std::uint32_t vertexShaderAssetID;
        std::uint32_t fragmentShaderAssetID;
    };

    struct Localisation {
        using asset_tag = AssetTag;

        using LocKey = std::variant<std::string, std::uint32_t>;
        struct LocKeyHash {
            using is_transparent = void;

            std::size_t operator()(const LocKey &key) const noexcept {
                if (const std::uint32_t *color = std::get_if<std::uint32_t>(&key))
                    return std::hash<std::uint32_t>{}(*color);
                return std::hash<std::string_view>{}(std::get<std::string>(key));
            }

            size_t operator()(const std::string_view sv) const {
                return std::hash<std::string_view>{}(sv);
            }

            size_t operator()(const std::uint32_t val) const noexcept {
                return std::hash<std::uint32_t>{}(val);
            }
        };
        struct LocKeyEqual {
            using is_transparent = void;

            bool operator()(const LocKey& lhs, const LocKey& rhs) const noexcept {
                return lhs == rhs;
            }
            bool operator()(const LocKey& lhs, const std::uint32_t rhs) const noexcept {
                if (const std::uint32_t* val = std::get_if<std::uint32_t>(&lhs)) return *val == rhs;
                return false;
            }
            bool operator()(const std::uint32_t lhs, const LocKey& rhs) const noexcept {
                if (const std::uint32_t* val = std::get_if<std::uint32_t>(&rhs)) return lhs == *val;
                return false;
            }
            bool operator()(const LocKey& lhs, const std::string_view rhs) const noexcept {
                if (const std::string *val = std::get_if<std::string>(&lhs)) return std::string_view(*val) == rhs;
                return false;
            }
            bool operator()(const std::string_view lhs, const LocKey& rhs) const noexcept {
                if (const std::string *val = std::get_if<std::string>(&rhs)) return lhs == std::string_view(*val);
                return false;
            }
        };
        using LocTable = std::unordered_map<LocKey, std::string, LocKeyHash, LocKeyEqual>;

        LocTable loc;
    };

    // Gameplay assets

    struct Save {
        using asset_tag = AssetTag;

        ee_data::GameState save;
    };

    struct ResourceList {
        using asset_tag = AssetTag;

        ee_economy::ResourceList resources;
    };

    struct FirmList {
        using asset_tag = AssetTag;

        ee_economy::FirmList firms;
    };

    struct ProductionRecipeList {
        using asset_tag = AssetTag;

        ee_economy::ProductionRecipeList recipes;
    };
}
