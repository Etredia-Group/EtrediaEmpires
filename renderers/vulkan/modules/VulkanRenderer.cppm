module;

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

export module VulkanRender;
import Render;
import views;
import vulkan_render;
import rmlui_vulkan;
import rml;
import std;

namespace ee_vulkan {
    template <typename T> using lazy = std::optional<T>;
    constexpr std::uint64_t TIMEOUT = std::numeric_limits<std::uint64_t>::max();

    struct DrawConfig {
        static constexpr vk::ClearValue CLEAR_COLOR = {.color = {std::array{1.0f, 1.0f, 1.0f, 1.0f}}};
        static constexpr vk::ClearValue CLEAR_DEPTH = {.depthStencil = {1.0f, 0}};
    };

    struct ProvinceMesh {
        lazy<vulkan_render::Buffer<
            glm::vec2,
            vk::BufferUsageFlagBits::eVertexBuffer,
            vulkan_render::BufferMode::STAGING_ACCESS
        >> vertices;
        lazy<vulkan_render::Buffer<
            std::uint32_t,
            vk::BufferUsageFlagBits::eIndexBuffer,
            vulkan_render::BufferMode::STAGING_ACCESS
        >> indices;
        lazy<vulkan_render::Buffer<
            ee_engine::ProvinceSSBO,
            vk::BufferUsageFlagBits::eStorageBuffer,
            vulkan_render::BufferMode::STAGING_ACCESS
        >> storage;

        lazy<vulkan_render::Buffer<
            vk::DrawIndexedIndirectCommand,
            vk::BufferUsageFlagBits::eIndirectBuffer,
            vulkan_render::BufferMode::STAGING_ACCESS
        >> indirectCommands;
    };
}

export namespace ee_vulkan {
    class VulkanRenderer final : public ee_engine::IRenderer {
    public:
        explicit VulkanRenderer(GLFWwindow *window);
        ~VulkanRenderer() override;

        void Init() override;
        void Shutdown() override;

        constexpr Rml::RenderInterface* getRenderInterface() noexcept override {return &*rmlRenderInterface;}

    protected:
        void DrawBegin() override;
        void DrawMap() override;
        void DrawUI() override;
        void DrawEnd() override;
        void ResizeWindow(int width, int height) override;

        void ApplyCamera() override;

        void UpdateProvinces(std::size_t offset, const std::vector<ee_engine::ProvinceSSBO>& data) override;

    private:
        const std::uint32_t MAX_FRAMES_IN_FLIGHT = 3;

        lazy<vulkan_render::Context> context;
        vulkan_render::Surface surface;
        lazy<vulkan_render::Swapchain> swapchain;
        lazy<vulkan_render::RenderCommands> renderCommands;
        lazy<vulkan_render::RenderPass> renderPass;
        lazy<vulkan_render::Pipeline> provincesPipeline;
        lazy<vulkan_render::Pipeline> rmlPipeline;
        lazy<vulkan_render::SyncManager> syncManager;

        lazy<vulkan_render::Texture<vulkan_render::TextureMode::DEPTH>> depth;
        lazy<vulkan_render::Uniform<glm::mat4>> viewProjectionUniform;
        ProvinceMesh provinces;

        lazy<ee_rml::RmlVulkan> rmlRenderInterface;

        std::size_t currentFrame = 0;
        lazy<vulkan_render::FrameContext> frameContext;

        void RecreateSwapchain();

        void LoadProvincesMaterial();
        void LoadRmlMaterial();
        void LoadProvinces();
        void UpdateData(std::uint32_t frame);
    };
}