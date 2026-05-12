/**
 * Factory partition module is responsible for construction of render objects
 */

export module vulkan_render:factory;
import :hardware;
import :resources;
import :control;
import :data;

export namespace vulkan_render {

    class ResourcesFactory {
    public:
        /**
         * Initialize frames swapchain
         * @param context Vulkan context with initialized device and surface capabilities.
         * @param surface surface origin. Extent will be clamped with frames size.
         */
        static Swapchain CreateSwapchain(const Context &context,Surface &surface);

    private:
        /**
         * Initialize images and views for swapchain frames
         * @param context Vulkan context with ready device.
         * @param swapchain frames target with initialized swapchain origin.
         * @param surface surface origin.
         */
        static void SetupFrames(const Context &context, Swapchain &swapchain, const Surface &surface);
    };

    class DataFactory {
    public:
        static Texture<TextureMode::RENDER> CreateRenderTexture(
            const Context &context,
            const RenderCommands &commands,
            std::uint32_t width, std::uint32_t height,
            std::span<std::byte> textureData
        );
    };

}