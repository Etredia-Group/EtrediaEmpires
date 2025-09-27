#pragma once
#include <thread>
#include <atomic>
#include <cstdint>
#include <vector>
#include <set>
#include <Boost/lockfree/spsc_queue.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
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

#define RENDER_PASS_BASE 438461662
#define RENDER_PASS_GUI

struct Transform
{
    float x = 0.0f, y = 0.0f;
    float scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;
};

struct Mesh
{
    uint32_t vertexCount = 0;
};

struct RenderObject
{
    // Multiple transforms for batching
    std::vector<Transform> transforms;
    uint8_t depth = 0;
    Mesh mesh; //TODO: mesh (vertex/index buffer)
    //TODO: texture

    AssetMaterial &material;
    //TODO: materialParams (uniforms)
};
bool operator < (const RenderObject& lhs, const RenderObject& rhs);

struct RenderCommand
{
    enum Type
    {
        NONE,

        REGISTER_OBJECTS,
        RESIZE_WINDOW, //STORE screenHeight and screenWidth BEFORE request
        SHUTDOWN
    } type = NONE;

    RenderObject *objects;
    size_t objectsSize;
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

struct DrawConfig
{
    VkCommandBufferBeginInfo commandBeginConfig {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pInheritanceInfo = nullptr
	};

    VkClearValue clearColor {{{1.0f, 1.0f, 1.0f, 1.0f}}};

    VkRenderPassBeginInfo renderPassBeginConfig {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .clearValueCount = 1,
        .pClearValues = &clearColor
    };

    VkPipelineStageFlags waitImageStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitConfig {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitDstStageMask = &waitImageStage,
		.commandBufferCount = 1,
		.signalSemaphoreCount = 1
	};

    VkPresentInfoKHR presentConfig {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .swapchainCount = 1,
        .pResults = nullptr
    };
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

    Renderer(GLFWwindow *window, const AssetMaterial& baseMaterial);
    ~Renderer();
    
    void Start();
    void Shutdown();

    void RegisterObjects(RenderObject* objects, size_t size);
    void ResizeWindow(int width, int height);

private:
    std::stop_token *stoken;
    std::jthread renderThread;
    boost::lockfree::spsc_queue<RenderCommand, boost::lockfree::capacity<512>> commands;

    GLFWwindow *window;

    VkInstance vulkan;
    VkSurfaceKHR surface;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkSwapchainKHR swapchain;
    VkRenderPass baseRenderPass;
    VkCommandPool commandPool = nullptr;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkImage> frames;
    std::vector<VkImageView> frameViews;
    std::vector<VkFramebuffer> baseFrameBuffers;
    std::vector<VkSemaphore> imageAvailable;
    std::vector<VkSemaphore> renderFinished;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;

    SurfaceInfo surfaceInfo;
    QueueFamiliesBundle queueFamilies;
    QueuesBundle queues;
    DrawConfig drawConfig;
    PipelineFactory *pipelineFactory;
    size_t currentFrame = 0;
    const float DEFAULT_QUEUE_PRIORITY = 1.0f;
    const AssetMaterial &baseMaterial;
    const size_t MAX_FRAMES_IN_FLIGHT = 2;

    std::set<RenderObject> objects;

    Rml::Context *uiContext;
    
    void InitVulkan();
    void SetupDevice();
    void SetupSwapchain();
    void RecreateSwapchain();
    void ReleaseSwapchain();
    void SetupRenderPasses();
    void SetupBuffers();
    void SetupSync();

    void Draw();

    void Threading();
    void Request(const RenderCommand &command);
    void Request(const RenderCommand::Type &type);
    void Init();
    void Loop();
    void ProcessCommand(const RenderCommand &command);
    void Release();

};