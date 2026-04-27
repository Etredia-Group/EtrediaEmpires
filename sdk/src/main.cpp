#include <cxxopts.hpp>
#include <nlohmann/json.hpp>
// IT'S NEEDED FOR MSVC COMPILER TO NOT CRASH WITH C1001
#include <rfl/capnproto.hpp>

import utils;
import AssetData;
import io;
import compile.json;
import compile.mapmesh;
import std;

using json = nlohmann::json;
using path = std::filesystem::path;

template <ee_assets::Asset TAsset>
void CompileJson(const ee_sdk::JsonCompiler &compiler, const path &relative, const path &src, const path &out) {
    LOG(INFO, "Start compiling %s...", relative.c_str());

    std::filesystem::create_directories(out/relative);
    for (const std::filesystem::directory_entry &asset : std::filesystem::directory_iterator(src/relative)) {
        if (!asset.is_regular_file()) continue;

        std::optional<json> data = ee_sdk::ReadJson(asset.path());
        if (!data.has_value()) continue;

        compiler.Compile(*data);
        ee_sdk::WriteAsset(out/relative/asset.path().stem().filename(), data->get<TAsset>());
    }
}

void CompileMaps(const ee_sdk::MapMeshCompiler &compiler, const path &relative, const path &src, const path &out) {
    LOG(INFO, "Start compiling %s...", relative.c_str());

    std::filesystem::create_directories(out/relative);
    for (const std::filesystem::directory_entry &asset : std::filesystem::directory_iterator(src/relative)) {
        if (!asset.is_regular_file()) continue;

        std::optional<std::vector<std::byte>> data = ee_sdk::ReadBinary(asset.path());
        if (!data.has_value()) continue;

        ee_sdk::WriteAsset(
            out/relative/asset.path().stem().filename(),
            ee_assets::AssetMapMesh{ .content = compiler.CompileBMP(*data) }
        );
    }
}

int main(const int argc, char **argv) {
    std::uint32_t status = 0;

    Logger::Init(L"/P7.Sink=Console");
    Logger::RegisterThread(L"Main");

    try {

        cxxopts::Options options(
           "Etredia Empires SDK",
           "This modding kit prepares your content assets for integration into game."
       );
        options.add_options()
            ("src", "Path to your mod source assets", cxxopts::value<path>())
            ("o,out", "Path to compiled assets", cxxopts::value<path>()->default_value("out"))
            ("h,help", "Print usage");

        if (
            const cxxopts::ParseResult cmd = options.parse(argc, argv);
            cmd.contains("help")
        ) {
            LOG(INFO, "\n%hs", options.help().c_str());
        }
        else {
            const path &src = cmd["src"].as<path>();
            const path &out = cmd["out"].as<path>();

            const ee_sdk::JsonCompiler jsonCompiler(src/"regions.json");
            CompileJson<ee_assets::AssetMaterial>(jsonCompiler, "materials", src, out);
            CompileJson<ee_assets::AssetSave>(jsonCompiler, "scenarios", src, out);

            constexpr ee_sdk::MapMeshCompiler mapMeshCompiler;
            CompileMaps(mapMeshCompiler, "map", src, out);
        }

    }
    catch (const std::exception &e)
    {
        LOG(CRITICAL, "%hs", e.what());
        status = 1;
    }

    Logger::UnregisterThread();
    Logger::Release();

    return status;
}