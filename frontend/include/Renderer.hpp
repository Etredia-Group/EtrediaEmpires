#pragma once
#include <thread>
#include <atomic>
#include <cstdint>
#include <Boost/lockfree/spsc_queue.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <RmlUi/Core.h>

#include "PipelineFactory.hpp"

#ifdef VALIDATION_LAYER_ON
    #define VALIDATION_LAYER "VK_LAYER_KHRONOS_validation"
#else
    #define VALIDATION_LAYER
#endif

// GPU properties priority for selection
#define GPU_DISCRETE_PRIORITY 1000
#define GPU_GRAPHICS_WITH_PRESENT_PRIORITY 500
#define GPU_B8G8R8_SRGB_PRIORITY 10

#define QUEUE_FAMILY_NO_VALUE UINT32_MAX

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
    uint32_t graphics = QUEUE_FAMILY_NO_VALUE;
    uint32_t present = QUEUE_FAMILY_NO_VALUE;
};

struct QueuesBundle
{
    VkQueue graphics;
    VkQueue present;
};

struct SurfaceInfo
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> availableFormats;
    std::vector<VkPresentModeKHR> availablePresentModes;

    VkSurfaceFormatKHR format;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;
};

class Renderer
{
public:
    std::atomic<bool> isRunning;
    std::atomic<int> screenWidth, screenHeight;

    Renderer(SDL_Window *window, const AssetMaterial& baseMaterial);
    ~Renderer();
    
    void Start();
    void Shutdown();

    void Request(const RenderCommand &command);
    void Request(const RenderCommand::Type &type);

private:
    std::stop_token *stoken;
    std::jthread renderThread;
    boost::lockfree::spsc_queue<RenderCommand, boost::lockfree::capacity<512>> commands;

    SDL_Window *window;

    VkInstance vulkan;
    VkSurfaceKHR surface;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> frames;
    std::vector<VkImageView> frameViews;

    SurfaceInfo surfaceInfo;
    QueueFamiliesBundle queueFamilies;
    QueuesBundle queues;
    const float defaultQueuePriority = 1.0f;
    const AssetMaterial &baseMaterial;
    PipelineFactory *pipelineFactory;

    Rml::Context *uiContext;
    
    void InitVulkan();
    void SetupDevice();
    void SetupSwapchain();

    void Threading();
    void Loop();
    void ProcessCommand(const RenderCommand &command);
    void Release();

};