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
	assetManager.LoadAsync(AssetInfo::MATERIAL, "assets/materials/helloworld.mat");
	AssetHandle baseMaterialHandle = assetManager.Find("assets/materials/helloworld.mat");
	AssetMaterial baseMaterial = assetManager.Get<AssetMaterial>(baseMaterialHandle);
	Renderer renderer(window, baseMaterial);
	renderer.Start();

	//Setup Engine
	Engine::assetManager = &assetManager;
	Engine::renderer = &renderer;

	// Test render obj
	RenderObject triangle {
		.transforms = {{.x = 0, .y = 0}},
		.mesh = {.vertexCount = 3},

		.material = baseMaterial,
	};
	renderer.RegisterObjects(&triangle, 1);

	// Process GLFW events
	while(!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	// Free memory
	LOG(INFO, "Exit...");
	renderer.Shutdown(); while(renderer.isRunning.load());
	assetManager.Shutdown(); while(assetManager.isRunning.load());
	glfwDestroyWindow(window);
	glfwTerminate();
	Logger::trace->Unregister_Thread(0);
	Logger::trace->Release();
	p7client->Release();

	return 0;
}