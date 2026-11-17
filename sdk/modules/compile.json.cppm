export module compile.json;
import utils;
import glaze;
import io;
import std;

export namespace ee_sdk {
    class JsonCompiler
    {
    public:
        explicit JsonCompiler(const std::filesystem::path &mappingsConfig) {
            for (
                const glz::json_t mappingsJson = ReadJson(mappingsConfig).value();
                auto &[key, value] : mappingsJson.get_object()
            ) {
                const char* color_str = value.get_string().c_str() + 1;
                std::uint8_t b = 0, g = 0, r = 0;
                std::uint32_t hex = 0;

                std::from_chars(color_str, color_str + 2, r, 16); color_str += 2;
                std::from_chars(color_str, color_str + 2, g, 16); color_str += 2;
                std::from_chars(color_str, color_str + 2, b, 16);

                hex = colorToHex(r, g, b);

                mappings[key] = hex;
            }
        }

        void Compile(glz::json_t &asset) const {
            CompileRecursive(asset, &CompileHashes);
            CompileRecursive(asset, std::bind_front(&JsonCompiler::CompileMappings, this));
            CompileRecursive(asset, &CompileColors);
        }

        /// Returns color (0xRRGGBB) registered for the name in regions.json, if any.
        std::optional<std::uint32_t> FindMapping(const std::string_view name) const {
            if (const auto it = mappings.find(name); it != mappings.end())
                return it->second;
            return std::nullopt;
        }

    private:
        struct StringHash {
            using is_transparent = void;
            std::size_t operator()(const std::string_view s) const noexcept {
                return std::hash<std::string_view>{}(s);
            }
        };

        std::unordered_map<std::string, std::uint32_t, StringHash, std::equal_to<>> mappings;

        static void CompileRecursive(
           glz::json_t &obj,
           const std::function<void(const std::basic_string<char> &key, glz::json_t &value)> &callback
       )
        {
            if (obj.is_array()) {for (auto &element : obj.get_array()) CompileRecursive(element, callback); return;}
            if (!obj.is_object()) {callback("", obj); return;}

            for (auto &[key, value] : obj.get_object())
            {
                if (value.is_object())
                    CompileRecursive(value, callback);
                else if (value.is_array())
                    for (auto &element : value.get_array())
                        CompileRecursive(element, callback);
                else callback(key, value);
            }
        }

        static bool NeedsCompileHash(const std::string &value) {
            return value.size() > 1 && value.starts_with("$");
        }

        static bool NeedsCompileMapping(const std::string &value) {
            return !value.starts_with("#") && !value.starts_with("$");
        }

        static void CompileHashes(const std::basic_string<char> &key, glz::json_t &value) {
            if (!value.is_string()) return;
            if (!NeedsCompileHash(value.get_string())) return;

            const std::filesystem::path linkPath = value.get_string();
            const auto linkPath8 = linkPath.generic_u8string();
            std::uint32_t hash = HashData(linkPath8.data() + 1, linkPath8.size() - 1);
            LOG(INFO, "COMPILED HASH [key:%hs]: Replace string [%s] with [%u] hash.",
                key.c_str(), linkPath.c_str(), hash);
            value = hash;
        }

        void CompileMappings(const std::basic_string<char> &key, glz::json_t &value) const {
            if (!value.is_string()) return;
            const std::string& value_string = value.get_string();
            if (!NeedsCompileMapping(value_string)) return;

            if (const std::optional<std::uint32_t> toValue = FindMapping(value_string)) {
                LOG(INFO, "COMPILED MAPPING [key:%hs]: Replace string [%hs] with [0x%X] (%u) value",
                    key.c_str(),
                    value_string.c_str(),
                    *toValue,
                    *toValue
                );
                value = *toValue;
            }
        }

        static void CompileColors(const std::basic_string<char> &key, glz::json_t &value) {
            if (!value.is_string()) return;
            const std::string &value_string = value.get_string();
            if (!value_string.starts_with('#')) return;

            const char *colorStr = value_string.c_str() + 1;
            const char *colorStrPointer = colorStr;
            std::uint8_t b = 0, g = 0, r = 0;
            std::uint32_t hex = 0;

            std::from_chars(colorStrPointer, colorStrPointer + 2, r, 16); colorStrPointer += 2;
            std::from_chars(colorStrPointer, colorStrPointer + 2, g, 16); colorStrPointer += 2;
            std::from_chars(colorStrPointer, colorStrPointer + 2, b, 16); colorStrPointer += 2;

            hex = colorToHex(r, g, b, true);

            LOG(INFO, "COMPILED COLOR [key:%hs]: Replace string [%hs] with 0x[%X] (RGB %u, %u, %u) color",
                key.c_str(), colorStr, hex, r, g, b);
            value = hex;
        }
    };
}