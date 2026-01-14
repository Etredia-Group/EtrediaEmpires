#include "Logger.h"
#include <GLFW/glfw3.h>

#include "compile_config.hpp"
#include "Engine.hpp"

IP7_Trace* Logger::trace;

int main(int argc, char **argv)
{
	// Setup P7 logger
	IP7_Client *p7client = P7_Create_Client(P7_SINK);
	P7_Set_Crash_Handler();
	Logger::trace = P7_Create_Trace(p7client, L"TC 1");
	Logger::trace->Register_Thread(L"Main", 0);

	LOG(INFO, "Etredia Empires, v%d.%d.%d", MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

	// Setup GLFW
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow *window = glfwCreateWindow(800, 600, "Etredia Empires", nullptr, nullptr);

	if (!window)
		LOG(CRITICAL, "GLFW Window failed");
	else LOG(INFO, "GLFW Window created");

	// Setup assets
	AssetManager assetManager;
	assetManager.Start();
	assetManager.LoadShaders();

	// Setup Renderer
	// assetManager.LoadAsync(AssetInfo::MATERIAL, "assets/materials/helloworld.mat");
	// AssetHandle baseMaterialHandle = assetManager.Find("assets/materials/helloworld.mat");
	// AssetMaterial baseMaterial = assetManager.Get<AssetMaterial>(baseMaterialHandle);
	// IRenderer renderer(window);
	// renderer.Start();

	//Setup Engine
	Engine::assetManager = &assetManager;
	//Engine::renderer = &renderer;
	
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
		Engine::renderer->ResizeWindow(width, height);
	});

	// Process GLFW events
	while(!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	// Free memory
	LOG(INFO, "Exit...");
	if (Engine::renderer != nullptr)
	{ // Wait for renderer to clean and shutdown
		Engine::renderer->Shutdown();
		while(Engine::renderer->GetThreadStatus());
	}
	if (Engine::assetManager != nullptr)
	{ // Wait for asset manager to clean and shutdown
		Engine::assetManager->Shutdown();
		while(Engine::assetManager->isRunning.load());
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	Logger::trace->Unregister_Thread(0);
	Logger::trace->Release();
	p7client->Release();

	return 0;
}