module;

#include <glm/gtc/matrix_transform.hpp>

module rmlui_vulkan;

// ReSharper disable once CppPassValueParameterByConstReference
ee_rml::RmlVulkan::RmlVulkan(RenderContext renderContext) :
    renderer(renderContext),
    sampler(vulkan_render::Sampler(renderContext.context)),
    defaultTexture(
        //std::move(
        vulkan_render::DataFactory::CreateRenderTexture(
            renderContext.context,
            renderContext.commands
        )
        //)
    ) {}

ee_rml::RmlVulkan::~RmlVulkan() {
    Flush(true);
}

Rml::CompiledGeometryHandle ee_rml::RmlVulkan::CompileGeometry(
    const Rml::Span<const Rml::Vertex> vertices,
    const Rml::Span<const std::int32_t> indices
) {
    GeometryMesh *mesh = Allocate<GeometryMesh>();
    mesh->vertices.emplace(renderer.context, vertices.size());
    mesh->indices.emplace(renderer.context, indices.size());

    std::ranges::copy(vertices, mesh->vertices->content.data());
    std::ranges::copy(indices, mesh->indices->content.data());

    renderer.commands.SubmitStagingBuffer(*mesh->vertices);
    renderer.commands.SubmitStagingBuffer(*mesh->indices);

    return reinterpret_cast<Rml::CompiledGeometryHandle>(mesh);
}

void ee_rml::RmlVulkan::RenderGeometry(
    const Rml::CompiledGeometryHandle geometry,
    const Rml::Vector2f translation,
    const Rml::TextureHandle texture
) {
    GeometryMesh *geometryMesh = reinterpret_cast<GeometryMesh*>(geometry);

    const Rml::TextureHandle cachedHandle = texture ? texture : static_cast<Rml::TextureHandle>(0);

    auto textureDescriptor = textures.find(cachedHandle);
    if (textureDescriptor == textures.end()) {
        textureDescriptor = textures.emplace(cachedHandle, renderer.pipeline.AllocateDescriptorSet()).first;

        const vulkan_render::RenderTexture *textureObject =
            texture ? reinterpret_cast<vulkan_render::RenderTexture*>(texture) : &defaultTexture;

        renderer.pipeline.UpdateDescriptors({
            vulkan_render::DescriptorInfo{
                .info = vk::DescriptorImageInfo{
                    .sampler = sampler.handle,
                    .imageView = textureObject->view,
                    .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
                },
                .bindingPosition = 1,
                .type = vk::DescriptorType::eCombinedImageSampler
            }
        }, textureDescriptor->second[0]);
    }

    frameContext->renderCommand->handle.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        renderer.pipeline.layout,
        0,
        {textureDescriptor->second[0]},
        {}
    );

    frameContext->renderCommand->handle.setViewport(0, {renderer.pipeline.viewport});
    frameContext->renderCommand->handle.setScissor(0, {renderer.pipeline.scissors});

    frameContext->renderCommand->handle.bindVertexBuffers(
        0,
        {geometryMesh->vertices->handle},
        {0}
    );
    frameContext->renderCommand->handle.bindIndexBuffer(
        geometryMesh->indices->handle,
        0,
        vk::IndexType::eUint32
    );

    RmlTransformGeometry transform {
        .mvp = glm::ortho(
            0.0f, static_cast<float>(renderer.surface.extent.width),
            0.0f, static_cast<float>(renderer.surface.extent.height)
        ) * currentTransform,
        .translate = glm::vec2(translation.x, translation.y)
    };

    frameContext->renderCommand->handle.pushConstants<RmlTransformGeometry>(
        renderer.pipeline.layout,
        vk::ShaderStageFlagBits::eVertex,
        0,
        {transform}
    );

    frameContext->renderCommand->handle.drawIndexed(
        geometryMesh->indices->content.size(),
        1,
        0,
        0,
        0
    );
}

void ee_rml::RmlVulkan::ReleaseGeometry(const Rml::CompiledGeometryHandle geometry) {
    Defer([this, mesh = reinterpret_cast<GeometryMesh*>(geometry)] {
        Deallocate(mesh);
    });
}

Rml::TextureHandle ee_rml::RmlVulkan::LoadTexture(Rml::Vector2i &texture_dimensions, const Rml::String &source) {
    const ee_assets::Texture &textureData = renderer.assetManager.FindOnce<ee_assets::Texture>(source);

    texture_dimensions.x = textureData.dimentionX;
    texture_dimensions.y = textureData.dimentionY;

    vulkan_render::RenderTexture *texture =
        Allocate<vulkan_render::RenderTexture>(
            std::move(
                vulkan_render::DataFactory::CreateRenderTexture(
                    renderer.context,
                    renderer.commands,
                    textureData.dimentionX,
                    textureData.dimentionY,
                    std::as_bytes(std::span{textureData.content, textureData.size}
                )
            )
        )
    );

    return reinterpret_cast<Rml::TextureHandle>(texture);
}

Rml::TextureHandle ee_rml::RmlVulkan::GenerateTexture(
    const Rml::Span<const std::uint8_t> source,
    const Rml::Vector2i source_dimensions
) {
    vulkan_render::RenderTexture *texture =
        Allocate<vulkan_render::RenderTexture>(
            std::move(
                vulkan_render::DataFactory::CreateRenderTexture(
                    renderer.context,
                    renderer.commands,
                    static_cast<std::uint32_t>(source_dimensions.x),
                    static_cast<std::uint32_t>(source_dimensions.y),
                    std::as_bytes(std::span{source.data(), source.size()}
                )
            )
        )
    );

    return reinterpret_cast<Rml::TextureHandle>(texture);
}

void ee_rml::RmlVulkan::ReleaseTexture(const Rml::TextureHandle texture) {
    auto descriptor = textures.extract(texture);
    Defer([
        this,
        descriptor = std::move(descriptor),
        object = reinterpret_cast<vulkan_render::RenderTexture*>(texture)
    ] {
        Deallocate(object);
    });
}

void ee_rml::RmlVulkan::EnableScissorRegion(const bool enable) {
    scissorEnabled = enable;
    if (enable) return;
    renderer.pipeline.scissors.extent = renderer.surface.extent;
    renderer.pipeline.scissors.offset = {.x = 0, .y = 0};
}

void ee_rml::RmlVulkan::SetScissorRegion(const Rml::Rectanglei region) {
    if (!scissorEnabled) return;

    const std::int32_t x = std::max(0, region.Position().x);
    const std::int32_t y = std::max(0, region.Position().y);
    std::uint32_t width  = std::max(0, region.Size().x);
    std::uint32_t height = std::max(0, region.Size().y);

    if (x + width  > renderer.surface.extent.width)width = renderer.surface.extent.width - x;
    if (y + height > renderer.surface.extent.height) height = renderer.surface.extent.height - y;

    renderer.pipeline.scissors.offset = {x, y};
    renderer.pipeline.scissors.extent = {width, height};
}

void ee_rml::RmlVulkan::SetTransform(const Rml::Matrix4f *transform) {
    if (transform != nullptr) {
        std::memcpy(&currentTransform[0][0], transform->data(), sizeof(glm::mat4));
    }
    else {
        currentTransform = glm::mat4(1.0f);
    }
}

void ee_rml::RmlVulkan::SetFrameContext(vulkan_render::FrameContext *frameContext) {
    this->frameContext = frameContext;
}

void ee_rml::RmlVulkan::CollectGarbage() {
    Flush(false);
}

void ee_rml::RmlVulkan::EndFrame() {
    frameCounter++;
}

void ee_rml::RmlVulkan::Defer(std::move_only_function<void()> destroy) {
    garbage.push_back({frameCounter, std::move(destroy)});
}

void ee_rml::RmlVulkan::Flush(bool all) {
    while (
        !garbage.empty() &&
        (all || frameCounter - garbage.front().frame >= renderer.framesInFlight)
    ) {
        garbage.front().destroy();
        garbage.pop_front();
    }
}
