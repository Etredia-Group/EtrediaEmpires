/**
 * Control partition module is responsible for configuration and usage of Vulkan operation objects
 * including command buffers, dynamic rendering, synchronization objects and so on.
 */

export module vulkan_render:control;
import :hardware;
import :workflow;
import :resources;
import :data;
import std;
import vulkan_hpp;

export namespace vulkan_render {

    /**
     * Command to perform an action on GPU such as rendering, copying, etc.
     *
     * Usage:
     * 1. instantiate command from pool.
     * 1. call vulkan_render::Command::Begin.
     * 1. call Vulkan API with vulkan_render::Command::handle.
     * 1. call vulkan_render::Command::Submit.
     */
    class Command {
    public:
        const Context &context;
        vk::raii::CommandBuffer handle;

        /**
         * Constructs wrapper for existing command.
         * @param context Vulkan context with ready device.
         * @param buffer source command buffer.
         */
        Command(const Context &context, vk::raii::CommandBuffer&& buffer);
        /**
         * Instantiates command buffer.
         * @param context Vulkan context with ready device.
         * @param pool commands source.
         */
        Command(const Context &context, const vk::raii::CommandPool &pool);

        Command(Command&&) noexcept = default;
        Command(const Command&) = delete;
        Command& operator=(const Command&) = delete;

        /**
         * Sets buffer in ready state to record command.
         */
        void Begin() const;
        /**
         * Sends recorded command to GPU queue.
         * @note vulkan_render::Command::Begin and
         * vulkan_render::Command::handle methods should be called earlier.
         * @param queue target.
         */
        void Submit(const vk::raii::Queue &queue) const;
    };

    /**
     * Set of Vulkan commands buffers for each frame in flight.
     */
    class RenderCommands {
    public:
        vk::raii::CommandPool graphicsPool;
        vk::raii::CommandPool transferPool;

        std::vector<Command> graphicsBuffers;
        std::vector<Command> transferBuffers;

        /**
         * Creates pools and initialize all command buffers.
         * @param context Vulkan context with ready device.
         * @param maxFramesInFlight amount of created buffers for each purpose.
         */
        RenderCommands(const Context &context, std::uint32_t maxFramesInFlight);

        /**
         * Transfers current buffer data from CPU to GPU.
         * @param buffer target.
         */
        void SubmitStagingBuffer(const BaseBuffer<BufferMode::STAGING_ACCESS> &buffer) const;
        /**
         * Updates texture with data from buffer.
         * @param buffer source data.
         * @param texture destination texture.
         */
        void SubmitTexture(
            const BaseBuffer<BufferMode::STATIC> &buffer,
            const Texture<TextureMode::PARTIAL> &texture
        ) const;
        /**
         * Changes texture image layout using pipeline barrier.
         * @param texture target.
         * @param from current layout.
         * @param to new layout.
         */
        void TranslateTextureLayout(
            const Texture<TextureMode::PARTIAL> &texture,
            const vk::ImageLayout &from,
            const vk::ImageLayout &to
        ) const;

    private:
        static vk::raii::CommandPool MakePool(const Context &context, std::uint32_t queueFamily);
        static std::vector<Command> MakeBuffers(
            const Context &context,
            const vk::raii::CommandPool &pool,
            std::uint32_t size
        );
    };

    /**
     * Controls synchronization between CPU and GPU using semaphores and fences.
     */
    class SyncManager {
    public:
        const Context &context;
        const std::uint32_t TIMEOUT;

        std::vector<vk::raii::Semaphore> imageAvailable;
        std::vector<vk::raii::Semaphore> renderFinished;
        std::vector<vk::raii::Fence> inFlightFences;
        std::vector<vk::Fence> imagesInFlight;

        /**
         * Inits synchronization objects for a given number of frames in flight.
         * @param context Vulkan context with ready device.
         * @param maxFramesInFlight maximum number of frames that may be processed concurrently.
         * @param timeout timeout in nanoseconds for fence waiting.
         */
        SyncManager(const Context &context, std::uint32_t maxFramesInFlight, std::uint64_t timeout);
        /**
         * Waits for the fence that guards the current frame submission to finish.
         * @param currentFrame index of the current in-flight frame (0..maxFramesInFlight-1).
         */
        void WaitInFlightFence(std::size_t currentFrame);
        /**
         * Waits for any previous frame fence that is still associated with a swapchain image,
         * then associates the current frame fence with that image.
         * @param currentFrame index of the current in-flight frame (0..maxFramesInFlight-1).
         * @param imageIndex index of the swapchain image that is about to be acquired.
         */
        void WaitImageInFlight(std::size_t currentFrame, std::uint32_t imageIndex);
        /**
         * Acquires the index of the next available swapchain image.
         * @param swapchain source to acquire the image.
         * @param currentFrame index of the current in-flight frame (0..maxFramesInFlight-1).
         * @param[out] imageIndex index of acquired image.
         * @return result code of operation.
         */
        vk::Result AcquireNextImage(
            const Swapchain &swapchain,
            std::size_t currentFrame,
            std::uint32_t &imageIndex
        ) const;
    };

    class FrameContext {
    public:
        const Command *renderCommand;
        std::uint32_t imageIndex;
        std::uint32_t currentFrame;

        FrameContext(
            const RenderPass &renderPass,
            const Surface &surface,
            const RenderCommands &renderCommands,
            std::uint32_t imageIndex,
            std::uint32_t currentFrame,
            std::span<vk::ClearValue> clearValues
        );
        void Complete() const;
    };
}