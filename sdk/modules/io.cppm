module;

#include <nlohmann/json.hpp>
#include <rfl/capnproto.hpp>

export module io;
import utils;
import AssetData;
import std;

using json = nlohmann::json;

export namespace ee_data
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        Subject,
        tag,
        color,
        regions
    )
}

export namespace ee_data
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        GameState,
        subjects
    )
}

export namespace ee_economy {
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        Resource,
        tag,
        baseCost
    )

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        EnterpriseDefinition,
        productionRecipe,
        constructionRecipe,
        constructionSpeed
    )
}

export namespace ee_assets
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        AssetSave,
        content
    )

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        AssetMaterial,
        vertexShaderAssetID,
        fragmentShaderAssetID
    )

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        AssetResourceList,
        content
    )

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        AssetEnterpriseList,
        content
    )
}

export namespace ee_sdk {
    std::optional<json> ReadJson(const std::filesystem::path &assetPath) {
        std::ifstream fin(assetPath);
        if (!fin.is_open()) {
            LOG(ERROR, "Could not open output file: [%s]", assetPath.c_str());
            return std::nullopt;
        }

        json data = json::parse(fin);

        fin.close();
        return data;
    }

    std::optional<std::vector<std::byte>> ReadBinary(const std::filesystem::path &assetPath) {
        std::ifstream fin(assetPath, std::ios::binary);
        if (!fin.is_open()) {
            LOG(ERROR, "Could not open output file: [%s]", assetPath.c_str());
            return std::nullopt;
        }

        std::vector<std::byte> data;

        // Read size
        fin.seekg(0, std::ios::end);
        const std::streamsize size = fin.tellg();
        fin.seekg(0, std::ios::beg);
        data.reserve(size);

        // Read content
        fin.read(reinterpret_cast<char *>(data.data()), size);
        fin.close();
        return data;
    }

    template <ee_assets::Asset TAsset>
    void WriteAsset(const std::filesystem::path &assetPath, const TAsset &asset) {
        std::ofstream fout(assetPath, std::ios::binary);
        if (!fout.is_open()) {
            LOG(ERROR, "Could not open input file: [%s]", assetPath.c_str());
            return;
        }

        const std::vector<char> data = rfl::capnproto::write(asset);

        fout.write(data.data(), data.size());
        fout.close();
    }

    constexpr std::uint32_t colorToHex(
        const std::uint8_t r,
        const std::uint8_t g,
        const std::uint8_t b,
        const bool isBGR = false
    ) {
        if (isBGR) {
            return
            static_cast<std::uint32_t>(b) << 16 |
            static_cast<std::uint32_t>(g) << 8 |
            static_cast<std::uint32_t>(r);
        }

        return
            static_cast<std::uint32_t>(r) << 16 |
            static_cast<std::uint32_t>(g) << 8 |
            static_cast<std::uint32_t>(b);
    }
}
