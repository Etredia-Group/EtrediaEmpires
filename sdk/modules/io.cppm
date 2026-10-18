module;

#include <ee_utils/serialization.hpp>

export module io;
import utils;
import glaze;
import AssetData;
import std;

export namespace ee_sdk {
    std::optional<glz::json_t> ReadJson(const std::filesystem::path &assetPath) {
        std::ifstream fin(assetPath);
        if (!fin.is_open()) {
            LOG(ERROR, "Could not open output file: [%s]", assetPath.c_str());
            return std::nullopt;
        }

        const std::string jsonStr{ std::istreambuf_iterator(fin), std::istreambuf_iterator<char>() };
        fin.close();

        glz::json_t data;
        if (glz::error_ctx error = glz::read_json(data, jsonStr)) {
            LOG(ERROR, "Could not parse JSON: [%s]. Error: [%d]", assetPath.c_str(), error.ec);
            return std::nullopt;
        }

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
