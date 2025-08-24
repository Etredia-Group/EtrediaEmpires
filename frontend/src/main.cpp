#include "Logger.h"
#include <SDL2/SDL.h>

#include "compile_config.hpp"
#include "AssetManager.hpp"
#include "Renderer.hpp"

IP7_Trace* Logger::trace;

int SDL_main(int argc, char **argv)
{
	// Setup P7 logger
	IP7_Client *p7client = P7_Create_Client(P7_SINK);
	P7_Set_Crash_Handler();
	Logger::trace = P7_Create_Trace(p7client, L"TC 1");
	Logger::trace->Register_Thread(L"Main", 0);

	LOG(INFO, "Etredia Empires, v%d.%d.%d", MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

	// Setup SDL
	bool quit = false;
	SDL_Event event;

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

	SDL_Window *window = SDL_CreateWindow(
		"Etredia Empires",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		800,
		600,
		SDL_WINDOW_VULKAN
	);
	SDL_StopTextInput();

	if (!window)
	{
		quit = true;
		LOG(CRITICAL, "SDL Window fails: %hs", SDL_GetError());
	}
	else LOG(INFO, "SDL Window created");

	// Setup assets
	AssetManager assetManager;
	assetManager.Start();
	assetManager.LoadShaders();

	// Setup Renderer
	Renderer renderer(window);
	renderer.Start();

	// Process SDL events
	while(!quit)
	{
		SDL_PollEvent(&event);

		switch(event.type)
		{
			case SDL_QUIT:
			{
				SDL_RestoreWindow(window);
				quit = true;
				break;
			}

			case SDL_WINDOWEVENT:
			{
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
					{
						renderer.screenWidth.store(event.window.data1);
						renderer.screenHeight.store(event.window.data2);
						renderer.Request(RenderCommand::RESIZE_WINDOW);
						break;
					}
				}

				break;
			}
		}
	}

	// Free memory
	LOG(INFO, "Exit...");
	renderer.Shutdown(); while(renderer.isRunning.load());
	assetManager.Shutdown(); while(assetManager.isRunning.load());
	SDL_DestroyWindow(window);
	SDL_Quit();
	Logger::trace->Unregister_Thread(0);
	Logger::trace->Release();
	p7client->Release();

	return 0;
}