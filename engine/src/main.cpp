#include "compile_config.hpp"
#include <GLFW/glfw3.h>

import utils;

import Engine;
import Actions;
import VulkanRender;
import GLFWInput;
import rmlui_glfw;
import std;

constexpr std::uint32_t SCREEN_WIDTH = 1920;
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
    Engine::assetManager = Allocate<AssetManager>();
    Engine::assetManager->Start();

    // Load assets
    Engine::assetManager->LoadDir<ee_assets::Shader>("assets/shaders");
    Engine::assetManager->LoadDir<ee_assets::Material>("assets/materials");
    Engine::assetManager->LoadAsync<ee_assets::MapMesh>("assets/map/ee.map");
    Engine::assetManager->LoadAsync<ee_assets::Save>("assets/scenarios/empires.save");
    Engine::assetManager->LoadDir<ee_assets::ResourceList>("assets/economy/resources");

    // Setup input manager
    Engine::input = Allocate<GLFWInput>(Actions::mappings);

    // Setup renderer
    Engine::renderer = reinterpret_cast<ee_engine::IRenderer*>(Allocate<ee_vulkan::VulkanRenderer>(window));

    ee_assets::MapMesh& regions = Engine::assetManager->FindOnce<ee_assets::MapMesh>("assets/map/ee.map");
    Engine::renderer->RegisterMapMesh(&regions.content);

    //TODO: ShaderRefs builder from set of AssetMaterial
    const ee_assets::Material& provincesMaterial =
        Engine::assetManager->FindOnce<ee_assets::Material>("assets/materials/provinces.mat");
    ee_assets::Shader& vertProv = Engine::assetManager->FindOnce<ee_assets::Shader>(
        provincesMaterial.vertexShaderAssetID);
    ee_assets::Shader& fragProv = Engine::assetManager->FindOnce<ee_assets::Shader>(
        provincesMaterial.fragmentShaderAssetID);
    const ee_assets::Material& rmlMaterial =
        Engine::assetManager->FindOnce<ee_assets::Material>("assets/materials/rmlui.mat");
    ee_assets::Shader& vertUI = Engine::assetManager->FindOnce<ee_assets::Shader>(
        rmlMaterial.vertexShaderAssetID);
    ee_assets::Shader& fragUI = Engine::assetManager->FindOnce<ee_assets::Shader>(
        rmlMaterial.fragmentShaderAssetID);
    Engine::renderer->RegisterMaterials({
        .vertProvinces = &vertProv,
        .fragProvinces = &fragProv,
        .vertRml = &vertUI,
        .fragRml = &fragUI
    });

    Engine::renderer->Init();
    Engine::renderer->SetCamera({
        .position = {1000.0f, 1000.0f},
        .zoom = 2000.0f
        //.zoom = 1.0f
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, const int width, const int height) {
        Engine::renderer->ResizeWindow(width, height);
    });

    // Setup Client
    Engine::client = Allocate<Client>();
    const AssetHandle save = Engine::assetManager->Find("assets/scenarios/empires.save");
    Engine::client->LoadSave(save);
    Engine::client->Init();
    Engine::client->Start();

    //Setup UI
    Engine::ui = Allocate<UI>(
        *Engine::renderer->getRenderInterface(),
        *Engine::input->getSystemInterface(),
        SCREEN_WIDTH, SCREEN_HEIGHT
    );
    Engine::renderer->SetUIContext(Engine::ui->getContext());
    Engine::input->SetUIHandlers(ee_rml::MakeInputHandlers(Engine::ui->getContext(), window));

    // Game loop
    ee_engine::RendererCommand renderCmdBuffer;
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
