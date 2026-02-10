import allocator;

import Engine;
import Actions;
import MagnumRender;
import GLFWInput;

#include "Logger.h"
#include "compile_config.hpp"
#include <GLFW/glfw3.h>
#include <vector>

IP7_Trace* Logger::trace;

int main(int argc, char **argv)
{
    // Setup P7 logger
    IP7_Client *p7client = P7_Create_Client(P7_SINK P7_LOGS_PATH);
    P7_Set_Crash_Handler();
    Logger::trace = P7_Create_Trace(p7client, L"TC 1");
    Logger::trace->Register_Thread(L"Main", 0);

    LOG(INFO, "Etredia Empires, v%d.%d.%d", MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

    // Setup GLFW
    glfwInit();

    // Vulkan only
    // glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow *window = glfwCreateWindow(800, 600, "Etredia Empires", nullptr, nullptr);

    if (!window)
        LOG(CRITICAL, "GLFW Window failed");
    else LOG(INFO, "GLFW Window created");

    glfwMakeContextCurrent(window);
    Engine::window = window;

    // Setup Asset Manager TODO: builder
    Engine::assetManager = Allocate<AssetManager>();
    Engine::assetManager->Start();
    Engine::assetManager->LoadDir(ee_assets::SHADER, "assets/shaders");
    Engine::assetManager->LoadDir(ee_assets::MATERIAL, "assets/materials");

    // Load assets
    Engine::assetManager->LoadAsync(ee_assets::MAP_MESH, "assets/map/ET.map");
    Engine::assetManager->LoadAsync(ee_assets::SAVE, "assets/scenarios/empires.save");

    // Setup input manager TODO: builder
    Engine::input = Allocate<GLFWInput>(Actions::mappings);

    // Setup renderer TODO: builder
    Engine::renderer = reinterpret_cast<ee_engine::IRenderer*>(Allocate<ee_magnum::MagnumRenderer>(window));

    ee_assets::AssetMapMesh &states = Engine::assetManager->FindOnce<ee_assets::AssetMapMesh>("assets/map/ET.map");
    Engine::renderer->RegisterMapMesh(&states.content);

    //TODO: ShaderRefs builder from set of AssetMaterial
    ee_assets::AssetMaterial &provincesMaterial =
            Engine::assetManager->FindOnce<ee_assets::AssetMaterial>("assets/materials/provinces.mat");
    ee_assets::AssetShader &vertProv = Engine::assetManager->FindOnce<ee_assets::AssetShader>(provincesMaterial.vertexShaderAssetID);
    ee_assets::AssetShader &fragProv = Engine::assetManager->FindOnce<ee_assets::AssetShader>(provincesMaterial.fragmentShaderAssetID);
    Engine::renderer->RegisterMaterials({
            .vertProvinces = &vertProv,
            .fragProvinces = &fragProv
    });

    Engine::renderer->Init();
    Engine::renderer->SetCamera({
            //.position = {1000.0f, 1000.0f},
            .zoom = 2000.0f
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
        Engine::renderer->ResizeWindow(width, height);
    });

    // Setup Client TODO: builder
    Engine::client = Allocate<Client>();
//    AssetHandle save = Engine::assetManager->Find("assets/scenarios/empires.save");
//    Engine::client->LoadSave(save);
//    Engine::client->Init();
    Engine::client->Start();

    // Game loop
    ee_engine::RendererCommand renderCmdBuffer; std::vector<ee_engine::RendererCommand> renderIncompletesBuffer;
    while(!glfwWindowShouldClose(window))
    {
        Engine::renderer->ProcessQueue(renderCmdBuffer, renderIncompletesBuffer);
        Engine::renderer->Draw();

        glfwSwapBuffers(window);
        Engine::input->Poll();
        glfwPollEvents();
    }

    // Free memory
    LOG(INFO, "Exit...");
    Engine::Shutdown();
    //TODO: check Unregister in other threads and make threaded module base class
    Logger::trace->Unregister_Thread(0);
    Logger::trace->Release();
    p7client->Release();

    return 0;
}

// TODO: Rename states to regions in old sources
// TODO: Remove unneccesary address providing like (const type &arg) for small primitives
// TODO: Use CRTP where possible in classes with virtual/override
// TODO: Use final in classes with override
// TODO: Implementations of API as partition modules
// TODO: Leaks detection