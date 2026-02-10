import AssetData;
import GameData;

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <xxhash.h>
#include <string>
#include <charconv>
#include <functional>

using json = nlohmann::json;

#define bytes(value) reinterpret_cast<const char*>(value)

namespace ee_data
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        Subject,
        tag,
        color,
        regions
    )
}

namespace ee_data
{
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        GameState,
        subjects
    )
}

namespace ee_assets
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
}

struct MappingEntry
{
    std::string_view name;
    uint32_t value;
};
static constexpr MappingEntry mappings[] = {
    #define MAP(name, value) {#name, ((value & 0x0000FF) << 16) | ( value & 0x00FF00) | ((value & 0xFF0000) >> 16)},
    #include "define/regions.def"
    #undef MAP
};

void CompileRecursive(json &obj, std::function<void(const std::basic_string<char> &key, json &value)> callback)
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

int main(int argc, char **argv)
{
    try
    {
        std::filesystem::path in = argv[1];
        std::filesystem::path out = argv[2];
        char* assetTypeStr = argv[3];

        std::ifstream fin(in);
        std::ofstream fout(out);
        ee_assets::AssetType assetType = static_cast<ee_assets::AssetType>(atoi(assetTypeStr));

        if (!fin.is_open()) {
            std::cout << "Error: Could not open input file: " << in << std::endl;
            return 1;
        }
        if (!fout.is_open()) {
            std::cout << "Error: Could not open output file: " << out << std::endl;
            return 1;
        }

        json data = json::parse(fin);

        // Compile hashes
        CompileRecursive(data, [](const std::basic_string<char> &key, json &value) {
            if (!value.is_string()) return;

            if (
                !(key == "tag") &&
                !key.ends_with("AssetID") &&
                !key.ends_with("Type")
            ) return;

            std::filesystem::path linkPath = value;
            auto linkPath8 = linkPath.generic_u8string();
            uint32_t assetId = XXH32(linkPath8.data(), linkPath8.size(), 0);

            std::cout << "Replace string [" << value << "] with [" << assetId << "]" << std::endl;
            value = assetId;
        });

        // Compile mappings
        CompileRecursive(data, [](const std::basic_string<char> &key, json &value) {
            if (!value.is_string()) return;
            const std::string& value_string = value.get_ref<const std::string&>();

            for (const MappingEntry &map : mappings)
            {
                if (value_string == map.name)
                {
                    std::cout << "Replace string [" << value << "] with [" << map.value << "]" << std::endl;
                    value = map.value;
                    break;
                }
            }
        });

        // Compile colors
        CompileRecursive(data, [](const std::basic_string<char> &key, json &value) {
            if (!value.is_string()) return;

            const char* color_str = value.get_ref<const std::string&>().c_str() + 1;
            uint8_t b = 0, g = 0, r = 0;
            uint32_t hex = 0;

            std::from_chars(color_str, color_str + 2, r, 16); color_str += 2;
            std::from_chars(color_str, color_str + 2, g, 16); color_str += 2;
            std::from_chars(color_str, color_str + 2, b, 16); color_str += 2;

            hex = static_cast<uint32_t>(r) << 16 | 
            static_cast<uint32_t>(g) << 8 | 
            static_cast<uint32_t>(b);

            std::cout << "Replace string [" << value << "] with [" << (int)r << "," << (int)g << "," << (int)b << "]" << std::endl;
            value = hex;
        });

        switch (assetType)
        {
            case ee_assets::MATERIAL:
            {
                ee_assets::AssetMaterial material = data.get<ee_assets::AssetMaterial>();
                fout.write(bytes(&material), sizeof(material));
                break;
            }
            
            case ee_assets::SAVE:
            {
                ee_assets::AssetSave save = data.get<ee_assets::AssetSave>();
                size_t size = 0;
                const char* data = bytes(save.Serialize(size));
                fout.write(data, size);
                break;
            }

            default:
              break;
        }

        fin.close();
        fout.close();    
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    

    return 0;
}