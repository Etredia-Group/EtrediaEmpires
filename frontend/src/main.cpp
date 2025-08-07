#ifdef _DEBUG
	#define P7_SINK L"/P7.Sink=Console"
#else
	#define P7_SINK L"/P7.Sink=FileTxt"
#endif

#include "P7_Client.h"
#include "P7_Trace.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <RmlUi/Core.h>

int SDL_main(int argc, char **argv)
{
	// Setup P7 logger
	IP7_Client *p7client = P7_Create_Client(P7_SINK);
	P7_Set_Crash_Handler();
	IP7_Trace *logger = P7_Create_Trace(p7client, L"TC 1");
	logger->Register_Thread(L"MainThread", 0);

	// Setup SDL
	bool quit = false;
	SDL_Event event;

	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window *window = SDL_CreateWindow(
		"Etredia Empires",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		800,
		600,
		SDL_WINDOW_VULKAN
	);

	if (!window)
	{
		quit = true;
		LOG(logger, CRITICAL, "SDL Window fails: %hs", SDL_GetError());
	}
	else LOG(logger, INFO, "SDL Window created");

	// Setup Vulkan
	VkInstance vulkan;
	
	uint32_t sdlExtensionsCount = 0;
	if ( !SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionsCount, nullptr) )
		LOG(logger, CRITICAL, "Fail to get SDL Vulkan extensions number! %hs", SDL_GetError());

	const char **sdlExtentions = new const char*[sdlExtensionsCount]();
	
	if ( !SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionsCount, sdlExtentions) )
		LOG(logger, CRITICAL, "Fail to load SDL Vulkan extentions! %hs", SDL_GetError());
	else LOG(logger, INFO, "Load %d SDL Vulkan extenstions", sdlExtensionsCount);
	
	VkInstanceCreateInfo vkCreateConfig = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.enabledLayerCount = 0,
		.enabledExtensionCount = sdlExtensionsCount,
		.ppEnabledExtensionNames = sdlExtentions
	};

	VkResult vkCreateResult = vkCreateInstance(&vkCreateConfig, nullptr, &vulkan);
	if (vkCreateResult != VK_SUCCESS)
		LOG(logger, CRITICAL, "Vulkan is not instantiated! Code: %d", vkCreateResult);
	else LOG(logger, INFO, "Vulkan is instantiated");
	
	delete[] sdlExtentions;

	//Setup RmlUi
	if (!Rml::Initialise())
		LOG(logger, CRITICAL, "RmlUi initialization failed!");
	else LOG(logger, INFO, "RmlUi initialized");

	int32_t screenWidth, screenHeight;
	SDL_GetWindowSize(window, &screenWidth, &screenHeight);
	Rml::Context *uiContext = Rml::CreateContext("default", Rml::Vector2i(screenWidth, screenHeight));

	// Process SDL events
	while(!quit)
	{
		SDL_WaitEvent(&event);

		switch(event.type)
		{
			case SDL_QUIT:
			{
				quit = true;
				break;
			}

			case SDL_WINDOWEVENT:
			{
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
					{
						uiContext->SetDimensions(Rml::Vector2i(event.window.data1, event.window.data2));
						break;
					}
				}

				break;
			}
		}

	}

	// Free memory
	LOG(logger, INFO, "Exit...");
	Rml::Shutdown();
	vkDestroyInstance(vulkan, nullptr);
	SDL_Quit();
	logger->Unregister_Thread(0);
	logger->Release();
	p7client->Release();

	return 0;
}