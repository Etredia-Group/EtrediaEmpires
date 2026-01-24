#include "Engine.hpp"
#include <ee_utils/allocator.hpp>

AssetManager* Engine::assetManager;
ee_engine::IRenderer* Engine::renderer;

void Engine::Shutdown()
{
    if (renderer != nullptr)
    {
        renderer->Shutdown();
        Deallocate(renderer);
    }
    renderer = nullptr;

    if (Engine::assetManager != nullptr)
	{ // Wait for asset manager to clean and shutdown
		Engine::assetManager->Shutdown();
		while(Engine::assetManager->isRunning.load());
	}

    if (assetManager != nullptr)
        Deallocate(assetManager);
    assetManager = nullptr;
}