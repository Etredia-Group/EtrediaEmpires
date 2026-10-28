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
        // Deep ocean color (linear). Normally hidden by the ocean plane drawn with the provinces.
        static constexpr vk::ClearValue CLEAR_COLOR = {.color = {std::array{0.007f, 0.122f, 0.474f, 1.0f}}};
        static constexpr vk::ClearValue CLEAR_DEPTH = {.depthStencil = {1.0f, 0}};
    };

    /**
     * Vertex of a province polygon. Normal is the outward miter of the province contour,
     * used by the shader to slightly grow polygons before clipping them against the province raster.
     */
    struct ProvinceVertex {
        glm::vec2 position;
        glm::vec2 normal;
    };

    /**
     * Per-frame data of the map shaders (std140 layout, see provinces-flat shaders).
     */
    struct MapUniform {
        glm::mat4 viewProjection;
        glm::mat4 inverseViewProjection;
        /// x, y: framebuffer size; z: world units per screen pixel; w: 1 if the target is sRGB.
        glm::vec4 viewport;
        /// x, y: world position of province raster texel (0, 0) center; z, w: province raster size in texels.
        glm::vec4 raster;
        /// x: map pixels per province raster texel; y: raster texels per coast field texel; z, w: coast field size.
        glm::vec4 coast;
        /// x: instance index of the ocean plane.
        glm::uvec4 info;
    };
    static_assert(sizeof(MapUniform) == 192, "MapUniform must match std140 layout of the map shaders");

    struct ProvinceMesh {
        lazy<vulkan_render::Buffer<
            ProvinceVertex,
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

        /// Province index + 1 for every map pixel, 0 for water (R16/R32 uint).
        lazy<vulkan_render::DataTexture> provinceIds;
        /// Signed distance to the coastline in province raster texels, positive in water (R16 float).
        lazy<vulkan_render::DataTexture> coastDistance;
        // TODO: vulkan_render
        vk::raii::Sampler nearestSampler = nullptr;
        vk::raii::Sampler linearSampler = nullptr;

        /// World position of the center of province raster texel (0, 0).
        glm::ivec2 rasterOrigin {0, 0};
        glm::uvec2 rasterSize {1, 1};
        /// Map pixels per province raster texel (more than 1 for maps exceeding texture limits).
        std::uint32_t rasterTexelSize = 1;
        glm::uvec2 coastFieldSize {1, 1};
        /// Province raster texels per coast distance field texel.
        std::uint32_t coastFieldFactor = 1;
        /// Index of the ocean plane in the storage buffer and in the instances of multi-draw.
        std::uint32_t oceanInstance = 0;
    };

    struct PickReadback {
        lazy<vulkan_render::Buffer<
            std::uint32_t,
            vk::BufferUsageFlagBits::eTransferDst,
            vulkan_render::BufferMode::CPU_ACCESS
        >> buffer;
        lazy<vulkan_render::UserCommands> commands;
        std::vector<bool> recorded;
        std::optional<glm::ivec2> requested;
        std::optional<glm::ivec2> lastTexel;
        bool idsPacked = false;
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
        constexpr ee_engine::ProvinceSSBO& getRegionData(std::uint32_t id) const noexcept override;

    protected:
        bool DrawBegin() override;
        void DrawMap() override;
        void DrawUI() override;
        void DrawEnd() override;
        void ResizeWindow(int width, int height) override;

        void ApplyCamera() override;

        void UpdateProvinces(std::size_t offset, const std::vector<ee_engine::ProvinceSSBO>& data) override;
        void RequestProvincePickAt(std::optional<glm::vec2> worldPos) override;

    private:
        const std::uint32_t MAX_FRAMES_IN_FLIGHT = 3;
        bool isSwapchainDirty = false;

        lazy<vulkan_render::Context> context;
        vulkan_render::Surface surface;
        lazy<vulkan_render::Swapchain> swapchain;
        lazy<vulkan_render::RenderCommands> renderCommands;
        lazy<vulkan_render::RenderPass> renderPass;
        lazy<vulkan_render::Pipeline> provincesPipeline;
        lazy<vulkan_render::Pipeline> rmlPipeline;
        lazy<vulkan_render::SyncManager> syncManager;

        lazy<vulkan_render::Texture<vulkan_render::TextureMode::DEPTH>> depth;
        lazy<vulkan_render::Uniform<MapUniform>> mapUniform;
        ProvinceMesh provinces;
        PickReadback pick {};

        lazy<ee_rml::RmlVulkan> rmlRenderInterface;

        std::size_t currentFrame = 0;
        lazy<vulkan_render::FrameContext> frameContext;

        void RecreateSwapchain();

        void LoadProvincesMaterial();
        void LoadRmlMaterial();
        void LoadProvinces();
        void LoadMapTextures(
            const std::vector<std::uint32_t> &ids,
            const std::vector<float> &coastField,
            std::uint32_t provincesCount
        );

        void UpdateData(std::uint32_t frame);
        void CreatePickReadback(bool idsPacked);
        void RecordPick(std::uint32_t frame);
        void ResolvePick(std::uint32_t frame);
    };
}