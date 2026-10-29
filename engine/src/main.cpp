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
    Engine::window = window;

    // Setup Asset Manager
    Engine::assetManager = Allocate<ee_engine::AssetManager>(0);
    Engine::assetManager->Start();

    // Start load assets
    Engine::assetManager->LoadDir<ee_assets::Shader>("assets/shaders");
    Engine::assetManager->LoadDir<ee_assets::Material>("assets/materials");
    Engine::assetManager->LoadDir<ee_assets::Texture>("assets/gfx");
    Engine::assetManager->LoadDir<ee_assets::ResourceList>("assets/economy/resources");

    Engine::assetManager->LoadAsync<ee_assets::MapMesh>("assets/map/ee.map");
    Engine::assetManager->LoadAsync<ee_assets::Save>("assets/scenarios/empires.save");

    // Setup renderer
    Engine::renderer = reinterpret_cast<ee_engine::IRenderer*>(Allocate<ee_vulkan::VulkanRenderer>(window));
    Engine::renderer->RegisterAssetManager(*Engine::assetManager);

    //TODO: ShaderRefs builder from set of AssetMaterial
    const ee_assets::Material &provincesMaterial =
        Engine::assetManager->FindOnce<ee_assets::Material>("assets/materials/provinces.mat");
    const ee_engine::AssetHandle vertProv = Engine::assetManager->Find(provincesMaterial.vertexShaderAssetID);
    const ee_engine::AssetHandle fragProv = Engine::assetManager->Find(provincesMaterial.fragmentShaderAssetID);
    const ee_assets::Material &rmlMaterial =
        Engine::assetManager->FindOnce<ee_assets::Material>("assets/materials/rmlui.mat");
    const ee_engine::AssetHandle vertUI = Engine::assetManager->Find(rmlMaterial.vertexShaderAssetID);
    const ee_engine::AssetHandle fragUI = Engine::assetManager->Find(rmlMaterial.fragmentShaderAssetID);
    Engine::renderer->RegisterMaterials({
        .vertProvinces = vertProv,
        .fragProvinces = fragProv,
        .vertRml = vertUI,
        .fragRml = fragUI
    });
    Engine::renderer->Init();

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, const int width, const int height) {
        Engine::renderer->ResizeWindow(width, height);
    });

    // Setup localization manager
    Engine::localizer = Allocate<Localizer>("ru", *Engine::assetManager);

    // Setup input manager
    Engine::input = Allocate<GLFWInput>(Actions::mappings);

    // Setup UI
    Engine::ui = Allocate<UI>(
        *Engine::renderer->getRenderInterface(),
        *Engine::input->getSystemInterface(),
        SCREEN_WIDTH, SCREEN_HEIGHT
    );
    Engine::renderer->SetUIContext(Engine::ui->getContext());
    Engine::input->SetUIHandlers(ee_rml::MakeInputHandlers(Engine::ui->getContext(), window));

    // Load game
    ee_engine::RendererCommand renderCmdBuffer;
    Engine::loadScreen = Allocate<LoadScreen>(LoadScreen::Context{
        .window = window,
        .input = *Engine::input,
        .renderer = *Engine::renderer,
        .localizer = *Engine::localizer,
        .ui = *Engine::ui,
        .renderCmdBuffer = renderCmdBuffer,
        .loadScreenScene = "assets/layout/fullscreen/loadscreen/loadscreen.rml"
    });

    LoadStages loadQueue = std::to_array<LoadStage>({
        {
            [] -> std::generator<float> {
               const ee_engine::AssetHandle regionsAsset = Engine::assetManager->Find("assets/map/ee.map");
               ee_assets::MapMesh& regions = Engine::assetManager->Get<ee_assets::MapMesh>(regionsAsset);
               Engine::renderer->RegisterMapMesh(&regions.content);
                   co_yield 0.1f;
               co_yield std::ranges::elements_of(ee_utils::ScaledCoroutine(
                   Engine::renderer->LoadProvinces(),
                   0.1f, 0.98f
               ));
               Engine::assetManager->Free(regionsAsset);
                   co_yield 0.99f;

               Engine::renderer->SetCamera({
                   .position = {11000.0f, 2800.0f},
                   .zoom = 2000.0f
               });
           }, "ProcessMap", 0.9f
        },

        {
            [] -> std::generator<float> {
                Engine::client = Allocate<Client>();
                const ee_engine::AssetHandle save = Engine::assetManager->Find("assets/scenarios/empires.save");
                Engine::client->LoadSave(save);
                co_yield 0.1f;

                co_yield std::ranges::elements_of(ee_utils::ScaledCoroutine(
                    Engine::client->Init(),
                    0.1f, 0.99f
                ));
                Engine::client->Start();
            }, "SetupClient", 0.1f
        },

    });
    Engine::loadScreen->Run(loadQueue, "assets/layout/overlay/overlay.rml");


    // Game loop
    while (!glfwWindowShouldClose(window)) {
        Engine::ui->Update();

        Engine::renderer->ProcessQueue(renderCmdBuffer);
        Engine::renderer->Draw();

        glfwSwapBuffers(window);
        Engine::input->Poll();
        glfwPollEvents();
    }

    // Free memory
    LOG(INFO, "Exit...");
    Engine::Shutdown();
    Logger::UnregisterThread();
    Logger::Release();

    return 0;
}

// TODO: Leaks detection
// TODO: Detach engine and engine-api from Rml
