#include "Engine.hpp"
#include <ee_utils/allocator.hpp>

GLFWwindow* Engine::window;

ee_engine::IInput* Engine::input;
AssetManager* Engine::assetManager;
ee_engine::IRenderer* Engine::renderer;

void Engine::Shutdown()
{
    if (input != nullptr)
        Deallocate(input);

    if (renderer != nullptr)
    {
        renderer->Shutdown();
        Deallocate(renderer);
    }
    renderer = nullptr;

    if (assetManager != nullptr)
	{ // Wait for asset manager to clean and shutdown
		assetManager->Shutdown();
		while(assetManager->isRunning.load()) {}
        Deallocate(assetManager);
	}
    assetManager = nullptr;

    glfwDestroyWindow(window);
	glfwTerminate();
}