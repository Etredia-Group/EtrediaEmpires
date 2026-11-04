#include "compile_config.hpp"
#include <GLFW/glfw3.h>

import utils;

import Engine;
import Actions;
import VulkanRender;
import GLFWInput;
import rmlui_glfw;
import std;

constexpr std::uint32_t SCREEN_WIDTH = 1200;
constexpr std::uint32_t SCREEN_HEIGHT = 800;

int main(int argc, char **argv) {
    // Setup P7 logger
    Logger::Init(P7_SINK P7_LOGS_PATH);
    Logger::RegisterThread(L"Main");

    LOG(INFO, "Etredia Empires, v%d.%d.%d", MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

    // Setup GLFW
    glfwInit();

    // Vulkan only
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Etredia Empires", nullptr, nullptr);

    if (!window)
        LOG(CRITICAL, "GLFW Window failed");
    else LOG(INFO, "GLFW Window created");

    glfwMakeContextCurrent(window);
    ee_engine::Engine::window = window;

    // Setup Asset Manager
    ee_engine::Engine::assetManager = Allocate<ee_engine::AssetManager>(0);
    ee_engine::Engine::assetManager->Start();

    // Start load assets
    ee_engine::Engine::assetManager->LoadDir<ee_assets::Shader>("assets/shaders");
    ee_engine::Engine::assetManager->LoadDir<ee_assets::Material>("assets/materials");
    ee_engine::Engine::assetManager->LoadDir<ee_assets::Texture>("assets/gfx");
    ee_engine::Engine::assetManager->LoadDir<ee_assets::ResourceList>("assets/economy/resources");

    ee_engine::Engine::assetManager->LoadAsync<ee_assets::MapMesh>("assets/map/ee.map");
    ee_engine::Engine::assetManager->LoadAsync<ee_assets::Save>("assets/scenarios/empires.save");

    // Setup renderer
    ee_engine::Engine::renderer = reinterpret_cast<ee_engine::IRenderer*>(Allocate<ee_vulkan::VulkanRenderer>(window));

    //TODO: ShaderRefs builder from set of AssetMaterial
    const ee_assets::Material &provincesMaterial =
        ee_engine::Engine::assetManager->FindOnce<ee_assets::Material>("assets/materials/provinces.mat");
    const ee_engine::AssetHandle vertProv = ee_engine::Engine::assetManager->Find(provincesMaterial.vertexShaderAssetID);
    const ee_engine::AssetHandle fragProv = ee_engine::Engine::assetManager->Find(provincesMaterial.fragmentShaderAssetID);
    const ee_assets::Material &rmlMaterial =
        ee_engine::Engine::assetManager->FindOnce<ee_assets::Material>("assets/materials/rmlui.mat");
    const ee_engine::AssetHandle vertUI = ee_engine::Engine::assetManager->Find(rmlMaterial.vertexShaderAssetID);
    const ee_engine::AssetHandle fragUI = ee_engine::Engine::assetManager->Find(rmlMaterial.fragmentShaderAssetID);
    ee_engine::Engine::renderer->RegisterMaterials({
        .vertProvinces = vertProv,
        .fragProvinces = fragProv,
        .vertRml = vertUI,
        .fragRml = fragUI
    });
    ee_engine::Engine::renderer->Init();

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, const int width, const int height) {
        ee_engine::Engine::renderer->ResizeWindow(width, height);
    });

    // Setup localization manager
    ee_engine::Engine::localizer = Allocate<ee_engine::Localizer>("ru");

    // Setup input manager
    ee_engine::Engine::input = Allocate<GLFWInput>(Actions::mappings);

    // Setup UI
    ee_engine::Engine::ui = Allocate<ee_engine::UI>(ee_engine::UI::Context{
        .renderer = *ee_engine::Engine::renderer->getRenderInterface(),
        .input = *ee_engine::Engine::input->getSystemInterface(),
        .width = SCREEN_WIDTH, .height = SCREEN_HEIGHT
    });
    ee_engine::Engine::renderer->SetUIContext(ee_engine::Engine::ui->getContext());
    ee_engine::Engine::input->SetUIHandlers(ee_rml::MakeInputHandlers(ee_engine::Engine::ui->getContext(), window));

    // Load game
    ee_engine::RendererCommand renderCmdBuffer;
    ee_engine::Engine::loadScreen = Allocate<ee_engine::LoadScreen>(ee_engine::LoadScreen::Context{
        .window = window,
        .renderCmdBuffer = renderCmdBuffer,
        .loadScreenScene = "fullscreen/loadscreen"
    });

    ee_engine::LoadStages loadQueue = std::to_array<ee_engine::LoadStage>({
        {
            [] -> std::generator<float> {
               const ee_engine::AssetHandle regionsAsset = ee_engine::Engine::assetManager->Find("assets/map/ee.map");
               ee_assets::MapMesh& regions = ee_engine::Engine::assetManager->Get<ee_assets::MapMesh>(regionsAsset);
               ee_engine::Engine::renderer->RegisterMapMesh(&regions.content);
                   co_yield 0.1f;
               co_yield std::ranges::elements_of(ee_utils::ScaledCoroutine(
                   ee_engine::Engine::renderer->LoadProvinces(),
                   0.1f, 0.98f
               ));
               ee_engine::Engine::assetManager->Free(regionsAsset);
                   co_yield 0.99f;

               ee_engine::Engine::renderer->SetCamera({
                   .position = {11000.0f, 4800.0f},
                   .zoom = 2000.0f
               });
           }, "ProcessMap", 0.9f
        },

        {
            [] -> std::generator<float> {
                ee_engine::Engine::client = Allocate<ee_engine::Client>();
                const ee_engine::AssetHandle save = ee_engine::Engine::assetManager->Find("assets/scenarios/empires.save");
                ee_engine::Engine::client->LoadSave(save);
                co_yield 0.1f;

                co_yield std::ranges::elements_of(ee_utils::ScaledCoroutine(
                    ee_engine::Engine::client->Init(),
                    0.1f, 0.99f
                ));
                ee_engine::Engine::client->Start();
            }, "SetupClient", 0.1f
        },

    });
    ee_engine::Engine::loadScreen->Run(
        loadQueue,
        "fullscreen/mainmenu"
    );


    // Game loop
    while (!glfwWindowShouldClose(window)) {
        ee_engine::Engine::ui->Update();

        ee_engine::Engine::renderer->ProcessQueue(renderCmdBuffer);
        ee_engine::Engine::renderer->Draw();

        glfwSwapBuffers(window);
        ee_engine::Engine::input->Poll();
        glfwPollEvents();
    }

    // Free memory
    LOG(INFO, "Exit...");
    ee_engine::Engine::Shutdown();
    Logger::UnregisterThread();
    Logger::Release();

    return 0;
}

// TODO: Leaks detection
// TODO: Detach engine and engine-api from Rml
