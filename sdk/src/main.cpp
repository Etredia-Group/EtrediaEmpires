#include <cxxopts.hpp>

import utils;
import glaze;
import AssetData;
import io;
import compile.json;
import compile.mapmesh;
import std;

using path = std::filesystem::path;

void IfNotExluded(const std::vector<path> &exluded, const path &a, const std::function<void()> &callback) {
    if (std::ranges::find(exluded, a) == exluded.end())
        callback();
    else LOG(INFO, "SKIP EXCLUDED [%s]", a.c_str());
}

template <ee_assets::Asset TAsset>
void CompileJson(const ee_sdk::JsonCompiler &compiler, const path &relative, const path &src, const path &out) {
    LOG(INFO, "Start compiling %s...", relative.c_str());

    std::filesystem::create_directories(out/relative);
    for (const std::filesystem::directory_entry &asset : std::filesystem::directory_iterator(src/relative)) {
        if (!asset.is_regular_file()) continue;

        std::optional<glz::json_t> data = ee_sdk::ReadJson(asset.path());
        if (!data.has_value()) continue;

        compiler.Compile(*data);
        TAsset assetInstance{};
        if (glz::error_ctx error = glz::read_json(assetInstance, *data)) {
            LOG(ERROR, "JSON schema mismatch: [%s]. Error: [%d]", asset.path().c_str(), error.ec);
        }
        ee_sdk::WriteAsset(out/relative/asset.path().stem().filename(), assetInstance);
    }
}

void CompileLocale(const ee_sdk::JsonCompiler &compiler, const path &relative, const path &src, const path &out) {
    LOG(INFO, "Start compiling %s...", relative.c_str());

    for (
        const std::filesystem::directory_entry &asset :
        std::filesystem::recursive_directory_iterator(src/relative)
    ) {
        if (!asset.is_regular_file()) continue;

        const path locFile = std::filesystem::relative(asset.path(), src/relative);
        std::filesystem::create_directories((out/relative/locFile).parent_path());

        std::optional<glz::json_t> data = ee_sdk::ReadJson(asset.path());
        if (!data.has_value()) continue;

        auto &loc = (*data)["loc"].get_object();
        ee_assets::Localisation locAsset;
        locAsset.loc.reserve(loc.size());

        //TODO: compile.loc.cppm
        for (auto &[key, value] : loc) {
            ee_assets::Localisation::LocKey locKey = key;

            if (key.size() > 1 && key[0] == '&') {
                const std::string_view regionName = std::string_view(key).substr(1);
                if (const std::optional<std::uint32_t> color = compiler.FindMapping(regionName)) {
                    locKey = *color;

                    LOG(INFO, "COMPILED COLOR KEY: Replace string [%hs] with [0x%X] (%u) color.",
                        std::string(regionName).c_str(), *color, *color);
                }
                else {
                    locKey = std::string(regionName);

                    LOG(ERROR, "No color mapping for region [%hs] in [%s]",
                        std::string(regionName).c_str(), asset.path().c_str());
                }
            }

            locAsset.loc[std::move(locKey)] = std::move(value.get_string());
        }

        ee_sdk::WriteAsset((out/relative/locFile).replace_extension(), locAsset);
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
            ee_assets::MapMesh{ .content = compiler.CompileBMP(*data) }
        );
    }
}

void CompileSPIRV(const path &compiler, const path &relative, const path &src, const path &out) {
    LOG(INFO, "Start compiling %s...", relative.c_str());

    for (
        const std::filesystem::directory_entry &asset :
        std::filesystem::recursive_directory_iterator(src/relative)
    ) {
        if (!asset.is_regular_file()) continue;

        const path shader = std::filesystem::relative(asset.path(), src/relative);
        std::filesystem::create_directories((out/relative/shader).parent_path());

        const std::string cmd = std::format("{} \"{}\" -O -o \"{}\"",
            compiler.string(),
            asset.path().string(),
            (out/relative/shader).string()
        );
        LOG(INFO, "Run [%hs]", cmd.c_str());

        if (
            const std::int32_t result = std::system(cmd.c_str());
            result == 0
        ) {
            LOG(INFO, "COMPILED SPIR-V SHADER: [%s]", (relative/shader).c_str());
        }
        else LOG(CRITICAL, "COMPILATION FAILED: [%s]. Result code: %d", (relative/shader).c_str(), result);
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
            ("e,exclude", "List of excluded asset types", cxxopts::value<std::vector<path>>()->default_value(""))
            ("E,exclude-path", "List of exluded asset paths", cxxopts::value<std::vector<path>>()->default_value(""))
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

            const std::vector<path> &excludedTypes = cmd["exclude"].as<std::vector<path>>();
            const std::vector<path> &excludedPaths = cmd["exclude-path"].as<std::vector<path>>();

            const ee_sdk::JsonCompiler jsonCompiler(src/"regions.json");
            IfNotExluded(excludedTypes, "materials", [&] {
                CompileJson<ee_assets::Material>(jsonCompiler, "materials", src, out);
            });
            IfNotExluded(excludedTypes, "scenarios", [&] {
                CompileJson<ee_assets::Save>(jsonCompiler, "scenarios", src, out);
            });
            IfNotExluded(excludedTypes, "economy", [&] {
                CompileJson<ee_assets::ResourceList>(jsonCompiler, "economy/resources", src, out);
                CompileJson<ee_assets::FirmList>(jsonCompiler, "economy/firms", src, out);
                CompileJson<ee_assets::ProductionRecipeList>(jsonCompiler, "economy/recipes", src, out);
            });
            IfNotExluded(excludedTypes, "localisation", [&] {
               CompileLocale(jsonCompiler, "localisation", src, out);
            });

            constexpr ee_sdk::MapMeshCompiler mapMeshCompiler;
            IfNotExluded(excludedTypes, "map", [&] {
                CompileMaps(mapMeshCompiler, "map", src, out);
            });

            IfNotExluded(excludedTypes, "shaders", [src, out] {
                CompileSPIRV("glslc", "shaders", src, out);
            });
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