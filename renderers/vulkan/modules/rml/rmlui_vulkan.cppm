module;

#include <glm/glm.hpp>

export module rmlui_vulkan;
import vulkan_render;
import AssetManager;
import utils;
import rml;
import std;

namespace ee_rml {
    template <typename T> using lazy = std::optional<T>;

    struct GeometryMesh {
        lazy<vulkan_render::Buffer<
            Rml::Vertex,
            vk::BufferUsageFlagBits::eVertexBuffer,
            vulkan_render::BufferMode::STAGING_ACCESS
        >> vertices;
        lazy<vulkan_render::Buffer<
            std::uint32_t,
            vk::BufferUsageFlagBits::eIndexBuffer,
            vulkan_render::BufferMode::STAGING_ACCESS
        >> indices;
    };
}

export namespace ee_rml {
    struct RmlVertexLayout {
        constexpr static std::size_t stride = sizeof(Rml::Vertex);
        constexpr static std::size_t offsetPos = offsetof(Rml::Vertex, position);
        constexpr static std::size_t offsetColor = offsetof(Rml::Vertex, colour);
        constexpr static std::size_t offsetUV = offsetof(Rml::Vertex, tex_coord);
    };

    struct RmlTransformGeometry {
        glm::mat4 mvp;
        glm::vec2 translate;
    };

    struct RenderContext {
        vulkan_render::Context &context;
        vulkan_render::Surface &surface;
        vulkan_render::RenderCommands &commands;
        vulkan_render::Pipeline &pipeline;

        ee_engine::AssetManager &assetManager;
        std::uint32_t framesInFlight;
    };

    class RmlVulkan final : public Rml::RenderInterface {
    public:
        explicit RmlVulkan(RenderContext renderContext);
        ~RmlVulkan() override;

        Rml::CompiledGeometryHandle CompileGeometry(
            Rml::Span<const Rml::Vertex> vertices,
            Rml::Span<const std::int32_t> indices
        ) override;

        void RenderGeometry(
            Rml::CompiledGeometryHandle geometry,
            Rml::Vector2f translation,
            Rml::TextureHandle texture
        ) override;

        void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

        Rml::TextureHandle LoadTexture(Rml::Vector2i &texture_dimensions, const Rml::String &source) override;

        Rml::TextureHandle GenerateTexture(
            Rml::Span<const std::uint8_t> source,
            Rml::Vector2i source_dimensions
        ) override;

        void ReleaseTexture(Rml::TextureHandle texture) override;

        void EnableScissorRegion(bool enable) override;

        void SetScissorRegion(Rml::Rectanglei region) override;

        void SetTransform(const Rml::Matrix4f *transform) override;

        void SetFrameContext(vulkan_render::FrameContext *frameContext);

        void CollectGarbage();
        void EndFrame();

    private:
        RenderContext renderer;
        vulkan_render::FrameContext *frameContext = nullptr;
        bool scissorEnabled = false;
        vulkan_render::Sampler sampler;
        std::unordered_map<Rml::TextureHandle, vk::raii::DescriptorSets> textures;
        vulkan_render::RenderTexture defaultTexture;
        glm::mat4 currentTransform = glm::mat4(1.0f);

        struct Garbage {
            std::uint64_t frame;
            std::move_only_function<void()> destroy;
        };
        std::deque<Garbage> garbage;
        std::uint64_t frameCounter = 0;

        void Defer(std::move_only_function<void()> destroy);
        void Flush(bool all);
    };

}