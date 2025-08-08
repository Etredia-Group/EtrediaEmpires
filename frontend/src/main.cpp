#include "Logger.h"
#include <SDL2/SDL.h>

#include "Renderer.hpp"


#ifdef _DEBUG
	#define P7_SINK L"/P7.Sink=Console"
#else
	#define P7_SINK L"/P7.Sink=FileTxt"
#endif

IP7_Trace* Logger::trace;

int SDL_main(int argc, char **argv)
{
	// Setup P7 logger
	IP7_Client *p7client = P7_Create_Client(P7_SINK);
	P7_Set_Crash_Handler();
	Logger::trace = P7_Create_Trace(p7client, L"TC 1");
	Logger::trace->Register_Thread(L"Main", 0);

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
		LOG(CRITICAL, "SDL Window fails: %hs", SDL_GetError());
	}
	else LOG(INFO, "SDL Window created");

	// Setup Renderer
	Renderer renderer(window);
	renderer.Start();

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
						//TODO: move to renderer
						//uiContext->SetDimensions(Rml::Vector2i(event.window.data1, event.window.data2));
						break;
					}
				}

				break;
			}
		}

	}

	// Free memory
	LOG(INFO, "Exit...");
	renderer.Stop(); while(renderer.isRunning) {/* */}
	SDL_Quit();
	Logger::trace->Unregister_Thread(0);
	Logger::trace->Release();
	p7client->Release();

	return 0;
}