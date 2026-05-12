/**
 * Do not include <RmlUi/Core.h> in module .cpp implementation
 * to prevent MSVC internal compiler error
 */

module;

#include <RmlUi/Core.h>

export module rmlui_vulkan;
import vulkan_render;
import utils;
import std;

namespace  ee_rml {
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

    struct RenderContext {
        vulkan_render::Context &context;
        vulkan_render::RenderCommands &commands;
    };

    class RmlVulkan final : public Rml::RenderInterface {
    public:
        explicit RmlVulkan(RenderContext &renderContext) : renderer(renderContext) {}

        Rml::CompiledGeometryHandle CompileGeometry(
            const Rml::Span<const Rml::Vertex> vertices,
            const Rml::Span<const std::int32_t> indices
        ) override {
            GeometryMesh *mesh = Allocate<GeometryMesh>();
            mesh->vertices.emplace(renderer.context, vertices.size());
            mesh->indices.emplace(renderer.context, indices.size());

            std::ranges::copy(vertices, mesh->vertices->content.data());
            std::ranges::copy(indices, mesh->indices->content.data());

            renderer.commands.SubmitStagingBuffer(*mesh->vertices);
            renderer.commands.SubmitStagingBuffer(*mesh->indices);

            return reinterpret_cast<Rml::CompiledGeometryHandle>(mesh);
        }

        void RenderGeometry(
            Rml::CompiledGeometryHandle geometry,
            Rml::Vector2f translation,
            Rml::TextureHandle texture
        ) override {

        }

        void ReleaseGeometry(const Rml::CompiledGeometryHandle geometry) override {
            Deallocate(reinterpret_cast<GeometryMesh*>(geometry));
        };

        Rml::TextureHandle LoadTexture(Rml::Vector2i &texture_dimensions, const Rml::String &source) override {return 0;}

        Rml::TextureHandle GenerateTexture(
            Rml::Span<const std::uint8_t> source,
            Rml::Vector2i source_dimensions
        ) override  {return 0;}

        void ReleaseTexture(Rml::TextureHandle texture) override {}

        void EnableScissorRegion(bool enable) override {}

        void SetScissorRegion(Rml::Rectanglei region) override {}

    private:
        RenderContext &renderer;
    };

}