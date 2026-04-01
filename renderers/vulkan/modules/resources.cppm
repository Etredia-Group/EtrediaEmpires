/**
 * Resources partition module is responsible for definition of
 * Vulkan resource objects including images, surface and so on.
 */

export module vulkan_render:resources;
import std;
import vulkan_hpp;

export namespace vulkan_render {
    /**
     * Vulkan objects associated with surface.
     * @note handle needs to be set manually according your window context.
     */
    struct Surface {
        vk::raii::SurfaceKHR handle = nullptr;
        vk::SurfaceFormatKHR format;
        vk::PresentModeKHR presentMode;
        vk::Extent2D extent;
    };

    /**
     * Vulkan objects associated with frames swapchain.
     * @note Can be created with vulkan_render::ResourcesFactory::CreateSwapchain.
     */
    struct Swapchain {
        vk::raii::SwapchainKHR handle;

        /**
         * Images used as targets to render frames.
         */
        std::vector<vk::Image> frames;
        /**
         * Views describing how the swapchain frames are accessed during rendering.
         */
        std::vector<vk::raii::ImageView> frameViews;
    };
}