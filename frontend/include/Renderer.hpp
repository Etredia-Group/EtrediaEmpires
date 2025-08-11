#pragma once
#include <thread>
#include <atomic>
#include <optional>
#include <Boost/lockfree/spsc_queue.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <RmlUi/Core.h>

#ifdef VALIDATION_LAYER_ON
    #define VALIDATION_LAYER "VK_LAYER_KHRONOS_validation"
#else
    #define VALIDATION_LAYER
#endif

// GPU properties priority for selection
#define GPU_DISCRETE_PRIORITY 1000

struct RenderCommand
{
    enum Type
    {
        NONE,

        //STORE screenHeight and screenWidth BEFORE request
        RESIZE_WINDOW,
        SHUTDOWN
    } type = NONE;
};

struct QueueFamiliesBundle
{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> presence;
};

class Renderer
{
private:
    std::stop_token *stoken;
    std::jthread renderThread;

    SDL_Window *window;

    VkInstance vulkan;
    VkPhysicalDevice gpu;
    VkDevice device;
    QueueFamiliesBundle queueFamilies;
    const float defaultQueuePriority = 1.0f;

    Rml::Context *uiContext;

    boost::lockfree::spsc_queue<RenderCommand, boost::lockfree::capacity<512>> commands;
    
    void InitVulkan();
    void SetupDevice();

    void Threading();
    void Loop();
    void ProcessCommand(const RenderCommand &command);
    void Release();

public:
    std::atomic<bool> isRunning;
    std::atomic<int> screenWidth, screenHeight;

    Renderer(SDL_Window *window);
    ~Renderer();
    
    void Start();
    void Shutdown();

    void Request(const RenderCommand &command);
    void Request(RenderCommand::Type type);
};