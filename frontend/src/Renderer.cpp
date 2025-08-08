#include "Renderer.hpp"
#include "Logger.h"
#include <allocator.hpp>

Renderer::Renderer(SDL_Window *window) : isRunning(false)
{
    this->window = window;
}

void Renderer::Start()
{
    isRunning = true;
    renderThread = std::jthread([this](std::stop_token stop) {
        this->stoken = &stop;
        Threading();
    });
}

void Renderer::Threading()
{
    Logger::trace->Register_Thread(L"Renderer", 0);
    LOG(INFO, "Renderer thread started");

    // Setup Vulkan
	uint32_t sdlExtensionsCount = 0;
	if ( !SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionsCount, nullptr) )
		LOG(CRITICAL, "Fail to get SDL Vulkan extensions number! %hs", SDL_GetError());

	const char **sdlExtentions = AllocateArray<const char*>(sdlExtensionsCount);
	
	if ( !SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionsCount, sdlExtentions) )
		LOG(CRITICAL, "Fail to load SDL Vulkan extentions! %hs", SDL_GetError());
	else LOG(INFO, "Load %d SDL Vulkan extenstions", sdlExtensionsCount);
	
	VkInstanceCreateInfo vkCreateConfig = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.enabledLayerCount = 0,
		.enabledExtensionCount = sdlExtensionsCount,
		.ppEnabledExtensionNames = sdlExtentions
	};

	VkResult vkCreateResult = vkCreateInstance(&vkCreateConfig, nullptr, &vulkan);
	if (vkCreateResult != VK_SUCCESS)
		LOG(CRITICAL, "Vulkan is not instantiated! Code: %d", vkCreateResult);
	else LOG(INFO, "Vulkan is instantiated");
	
	DeallocateArray(sdlExtentions);

	//Setup RmlUi
	if (!Rml::Initialise())
		LOG(CRITICAL, "RmlUi initialization failed!");
	else LOG(INFO, "RmlUi initialized");

	int32_t screenWidth, screenHeight;
	SDL_GetWindowSize(window, &screenWidth, &screenHeight);
	uiContext = Rml::CreateContext("default", Rml::Vector2i(screenWidth, screenHeight));
    if (!uiContext)
        LOG(CRITICAL, "RmlUi context creation failed!");
    else LOG(INFO, "RmlUi context created");

    Loop();

    Release();
}

void Renderer::Loop()
{
    while (!stoken->stop_requested())
    {
        /* */
        LOG(DEBUG, "...");
        Sleep(1000);
    }
}

void Renderer::Stop()
{
    renderThread.request_stop();
}

void Renderer::Release()
{
	Rml::Shutdown();
	vkDestroyInstance(vulkan, nullptr);

    isRunning = false;

    LOG(INFO, "Renderer released");
}