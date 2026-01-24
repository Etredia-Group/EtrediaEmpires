#include "Logger.h"

#include <ee_magnum/Renderer.hpp>
#include <ee_utils/allocator.hpp>

#include "compile_config.hpp"
#include "Engine.hpp"

#include <GLFW/glfw3.h>

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

	// Setup Asset Manager
	Engine::assetManager = Allocate<AssetManager>();
	Engine::assetManager->Start();
	Engine::assetManager->LoadDir(AssetInfo::SHADER, "assets/shaders");
	Engine::assetManager->LoadDir(AssetInfo::MATERIAL, "assets/materials");

	// Load assets
	Engine::assetManager->LoadAsync(AssetInfo::MAP_MESH, "assets/map/states.map");

	// Setup renderer TODO: builder
	Engine::renderer = reinterpret_cast<ee_engine::IRenderer*>(Allocate<ee_magnum::MagnumRenderer>(window));
	
	ee_assets::AssetMapMesh &states = Engine::assetManager->FindOnce<ee_assets::AssetMapMesh>("assets/map/states.map");
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
		.position = {1000.0f, 1000.0f},
		.zoom = 2000.0f
	});
	
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
		Engine::renderer->ResizeWindow(width, height);
	});

	// Game loop
	while(!glfwWindowShouldClose(window))
	{
		Engine::renderer->Draw();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Free memory
	LOG(INFO, "Exit...");
	Engine::Shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();
	//TODO: check Unregister in other threads and make threaded module base class
	Logger::trace->Unregister_Thread(0);
	Logger::trace->Release();
	p7client->Release();

	return 0;
}