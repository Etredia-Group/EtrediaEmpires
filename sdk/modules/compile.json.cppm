module;

#include <xxhash.h>
#include <nlohmann/json.hpp>

export module compile.json;
import utils;
import io;
import std;

using json = nlohmann::json;

export namespace ee_sdk {
    class JsonCompiler
    {
    public:
        explicit JsonCompiler(const std::filesystem::path &mappingsConfig) {
            for (
                const json mappingsJson = ReadJson(mappingsConfig);
                auto &[key, value] : mappingsJson.items()
            ) {
                const char* color_str = value.get_ref<const std::string&>().c_str() + 1;
                std::uint8_t b = 0, g = 0, r = 0;
                std::uint32_t hex = 0;

                std::from_chars(color_str, color_str + 2, r, 16); color_str += 2;
                std::from_chars(color_str, color_str + 2, g, 16); color_str += 2;
                std::from_chars(color_str, color_str + 2, b, 16);

                hex = colorToHex(r, g, b);

                mappings[key] = hex;
            }
        }

        void Compile(json &asset) const {
            CompileRecursive(asset, CompileHashes);
            CompileRecursive(asset, std::bind_front(&JsonCompiler::CompileMappings, this));
            CompileRecursive(asset, &CompileColors);
        }

    private:
        std::unordered_map<std::string, std::uint32_t> mappings;

        static void CompileRecursive(
           json &obj,
           const std::function<void(const std::basic_string<char> &key, json &value)> &callback
       )
        {
            if (obj.is_array()) {for (auto &element : obj) CompileRecursive(element, callback); return;}
            if (!obj.is_object()) {callback("", obj); return;}

            for (auto &[key, value] : obj.items())
            {
                if (value.is_object())
                    CompileRecursive(value, callback);
                else if (value.is_array())
                    for (auto &element : value)
                        CompileRecursive(element, callback);
                else callback(key, value);
            }
        }

        //TODO: extern config
        static bool NeedsCompile(const std::basic_string<char> &key) {
            return
                key == "tag" ||
                key.ends_with("AssetID") ||
                key.ends_with("Type") ||
                key.ends_with("Recipe");
        }

        static void CompileHashes(const std::basic_string<char> &key, json &value) {
            if (!value.is_string()) return;
            if (!NeedsCompile(key)) return;

            const std::filesystem::path linkPath = value;
            const auto linkPath8 = linkPath.generic_u8string();
            std::uint32_t assetId = XXH32(linkPath8.data(), linkPath8.size(), 0);

            LOG(INFO, "COMPILED HASH: Replace string [%s] with [%d] asset ID", linkPath.c_str(), assetId);
            value = assetId;
        }

        void CompileMappings(const std::basic_string<char> &, json &value) const {
            if (!value.is_string()) return;
            const std::string& value_string = value.get_ref<const std::string&>();

            for (const auto &[name, toValue] : mappings)
            {
                if (value_string == name)
                {
                    LOG(INFO, "COMPILED MAPPING: Replace string [%hs] with [0x%X] (%d) value",
                        value_string.c_str(),
                        toValue,
                        toValue
                    );
                    value = toValue;
                    break;
                }
            }
        }

        static void CompileColors(const std::basic_string<char> &, json &value) {
            if (!value.is_string()) return;

            const char *colorStr = value.get_ref<const std::string&>().c_str() + 1;
            const char *colorStrPointer = colorStr;
            std::uint8_t b = 0, g = 0, r = 0;
            std::uint32_t hex = 0;

            std::from_chars(colorStrPointer, colorStrPointer + 2, r, 16); colorStrPointer += 2;
            std::from_chars(colorStrPointer, colorStrPointer + 2, g, 16); colorStrPointer += 2;
            std::from_chars(colorStrPointer, colorStrPointer + 2, b, 16); colorStrPointer += 2;

            hex = colorToHex(r, g, b, true);

            LOG(INFO, "COMPILED COLOR: Replace string [%hs] with 0x[%X] (RGB %d, %d, %d) color", colorStr, hex, r, g, b);
            value = hex;
        }
    };
}