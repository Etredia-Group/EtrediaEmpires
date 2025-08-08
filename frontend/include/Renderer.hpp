#pragma once
#include <thread>
#include <atomic>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <RmlUi/Core.h>

class Renderer
{
private:
    std::stop_token *stoken;
    std::jthread renderThread;

    SDL_Window *window;
    VkInstance vulkan;
    Rml::Context *uiContext;

    void Threading();
    void Loop();
    void Release();

public:
    std::atomic<bool> isRunning;

    Renderer(SDL_Window *window);
    
    void Start();
    void Stop();
};