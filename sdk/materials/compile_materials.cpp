#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <xxhash.h>
#include <string>
#include <ee_engine_api/AssetData.hpp>

using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    AssetMaterial,
    vertexShaderAssetID,
    fragmentShaderAssetID
)

int main(int argc, char **argv)
{
    try
    {
        std::filesystem::path in = argv[1];
        std::filesystem::path out = argv[2];


        std::ifstream fin(in);
        std::ofstream fout(out);

        if (!fin.is_open()) {
            std::cout << "Error: Could not open input file." << std::endl;
            return 1;
        }
        if (!fout.is_open()) {
            std::cout << "Error: Could not open output file." << std::endl;
            return 1;
        }

        json data = json::parse(fin);

        for (auto &[key, value] : data.items())
        {
            if (
                !key.ends_with("AssetID") &&
                !key.ends_with("Type")
            ) continue;

            std::filesystem::path linkPath = data[key];
            auto linkPath8 = linkPath.generic_u8string();
            uint32_t assetId = XXH32(linkPath8.data(), linkPath8.size(), 0);
            data[key] = assetId;
        }

        AssetMaterial material = data.get<AssetMaterial>();
        fout.write(reinterpret_cast<const char*>(&material), sizeof(material));

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