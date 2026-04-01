/**
 * Control partition module is responsible for configuration and usage of Vulkan operation objects
 * including command buffers, dynamic rendering, synchronization objects and so on.
 */

export module vulkan_render:control;
import :hardware;
import :resources;
import :data;
import std;
import vulkan_hpp;

export namespace vulkan_render {

    class Command {
    public:
        const Context &context;
        vk::raii::CommandBuffer handle;

        Command(const Context &context, vk::raii::CommandBuffer&& buffer);
        Command(const Context &context, const vk::raii::CommandPool &pool);

        Command(Command&&) noexcept = default;
        Command(const Command&) = delete;
        Command& operator=(const Command&) = delete;

        void Begin() const;
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
         * Create pools and initialize all command buffers.
         * @param context Vulkan context with ready device.
         * @param maxFramesInFlight amount of created buffers for each purpose.
         */
        RenderCommands(const Context &context, std::uint32_t maxFramesInFlight);

        void SubmitStagingBuffer(const BaseBuffer<BufferMode::STAGING_ACCESS> &buffer) const;

    private:
        static vk::raii::CommandPool MakePool(const Context &context, std::uint32_t queueFamily);
        static std::vector<Command> MakeBuffers(
            const Context &context,
            const vk::raii::CommandPool &pool,
            std::uint32_t size
        );
    };

    //TODO: sync2 with timeline-semaphore(s)
    class SyncManager {
    public:
        const Context &context;
        const std::uint32_t TIMEOUT;

        std::vector<vk::raii::Semaphore> imageAvailable;
        std::vector<vk::raii::Semaphore> renderFinished;
        std::vector<vk::raii::Fence> inFlightFences;
        std::vector<vk::Fence> imagesInFlight;

        SyncManager(const Context &context, std::uint32_t maxFramesInFlight, std::uint64_t timeout);
        void WaitInFlightFence(std::size_t currentFrame);
        void WaitImageInFlight(std::size_t currentFrame, std::uint32_t imageIndex);
        vk::Result AcquireNextImage(
            const Swapchain &swapchain,
            std::size_t currentFrame,
            std::uint32_t &imageIndex
        ) const;
    };
}