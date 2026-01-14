#pragma once
#include <thread>
#include <atomic>
#include <cstdint>
#include <vector>
#include <set>
#include <Boost/lockfree/spsc_queue.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "GPU_Types/GPU_BaseTypes.hpp"

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

#define INDICES_SIZE uint16_t
#define INDICES_SIZE_VK VK_INDEX_TYPE_UINT16

struct Mesh
{
    size_t verticesBufferPosition = 0;
    size_t indicesBufferPosition = 0;
    std::vector<GPU_Vertex> vertices;
    std::vector<INDICES_SIZE> indices;
};

struct MaterialParams
{
    VkDescriptorSet descriptors;
    //TODO: push-constants

    void Set(const char** param, const char** property = nullptr);
};

struct RenderObject
{
    uint32_t engineId = UINT32_MAX;

    // Multiple transforms for batching
    std::vector<GPU_Transform> transforms;
    uint8_t depth = 0;
    Mesh mesh;
    bool isUI = false;
    //TODO: texture

    AssetMaterial &material;
    MaterialParams materialParams;
};
bool operator < (const RenderObject& lhs, const RenderObject& rhs);

struct RenderCommand
{
    enum Type
    {
        NONE,

        REGISTER_OBJECTS,
        RESIZE_WINDOW,
        SHUTDOWN
    } type = NONE;

    RenderObject *objects;
    size_t objectsSize;
};

struct QueueFamiliesBundle
{
    uint32_t transfer = QUEUE_FAMILY_NO_VALUE;
    uint32_t graphics = QUEUE_FAMILY_NO_VALUE;
    uint32_t present = QUEUE_FAMILY_NO_VALUE;
};

struct QueuesBundle
{
    VkQueue transfer;
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

    VkVertexInputBindingDescription vertexBindingConfig {
        .binding = 0,
        .stride = sizeof(GPU_Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
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
    Renderer(GLFWwindow *window, const AssetMaterial& baseMaterial, const size_t &vertexBufferSize, const size_t &indexBufferSize);
    ~Renderer();
    
    void Start();
    void Shutdown();
    
    bool GetThreadStatus();
    bool GetDrawStatus();

    void RegisterObjects(RenderObject* objects, size_t size);
    void ResizeWindow(int width, int height);

private:
    std::atomic<bool> isRunning;
    std::atomic<bool> isDrawing;
    std::atomic<int> screenWidth, screenHeight;

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
    VkCommandPool graphicsCommandPool = nullptr;
    VkCommandPool transferCommandPool = nullptr;
    VkDeviceMemory worldVerticesMemory;
    VkDeviceMemory worldIndicesMemory;
    VkDeviceMemory stagingWorldVerticesMemory;
    VkDeviceMemory stagingWorldIndicesMemory;
    VkDeviceMemory uiVerticesMemory;
    VkDeviceMemory uiIndicesMemory;
    VkBuffer worldVertices;
    VkBuffer worldIndices;
    VkBuffer stagingWorldVertices;
    VkBuffer stagingWorldIndices;
    VkBuffer uiVertices;
    VkBuffer uiIndices;
    std::vector<VkDeviceMemory> worldUniformsMemory;
    std::vector<VkBuffer> worldUniforms;
    std::vector<VkDeviceMemory> uiUniformsMemory;
    std::vector<VkBuffer> uiUniforms;
    std::vector<VkCommandBuffer> graphicsCommandBuffers;
    std::vector<VkCommandBuffer> transferCommandBuffers;
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
    PipelineBinding pipelineBinding;
    size_t currentFrame = 0;

    GPU_Buffer<GPU_Vertex> worldVerticesBinded;
    GPU_Buffer<INDICES_SIZE> worldIndicesBinded;
    GPU_Buffer<GPU_Vertex> uiVerticesBinded;
    GPU_Buffer<INDICES_SIZE> uiIndicesBinded;

    const float DEFAULT_QUEUE_PRIORITY = 1.0f;
    const AssetMaterial &baseMaterial;
    const size_t MAX_FRAMES_IN_FLIGHT = 2;
    const size_t VERTEX_BUFFER_SIZE;
    const size_t INDEX_BUFFER_SIZE;

    std::set<RenderObject> objects;

    //Rml::Context *uiContext;
    
    void InitVulkan();
    void SetupDevice();
    void SetupSwapchain();
    void RecreateSwapchain();
    void ReleaseSwapchain();
    void SetupRenderPasses();
    void SetupPipelines();
    void SetupObjectsBuffers();
    void SetupRenderBuffers();
    void SetupSync();

    void Draw();
    void CopyBuffer(VkBuffer src, size_t offset, VkBuffer dst, VkDeviceSize size);

    void Threading();
    void Request(const RenderCommand &command);
    void Request(const RenderCommand::Type &type);
    void Init();
    void Loop();
    void ProcessCommand(const RenderCommand &command);
    void Release();

};