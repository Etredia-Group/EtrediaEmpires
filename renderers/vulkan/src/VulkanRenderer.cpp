module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

module VulkanRender;
import utils;

ee_vulkan::VulkanRenderer::VulkanRenderer(GLFWwindow *window) : IRenderer(window) {
    std::uint32_t glfwExtensionsCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

    context.emplace(
        "EtrediaEmpires",
        std::vector(glfwExtensions, glfwExtensions + glfwExtensionsCount),
        std::vector{"VK_LAYER_KHRONOS_validation"}
    );

    VkSurfaceKHR vulkanSurface;
    glfwCreateWindowSurface(*context->vulkan, window, nullptr, &vulkanSurface);
    surface.handle = vk::raii::SurfaceKHR(context->vulkan, vulkanSurface);
    surface.extent = {
        .width = static_cast<std::uint32_t>(screenWidth),
        .height = static_cast<std::uint32_t>(screenHeight)
    };

    context->SetupPhysicalDevice(surface, {}, surface.format, surface.presentMode);
    context->SetupLogicalDevice({vk::KHRSwapchainExtensionName});

    swapchain.emplace(
        vulkan_render::ResourcesFactory::CreateSwapchain(*context, surface)
    );
    renderCommands.emplace(*context, MAX_FRAMES_IN_FLIGHT);
    syncManager.emplace(*context, MAX_FRAMES_IN_FLIGHT, TIMEOUT);

    LOG(INFO, "Renderer created");
}

ee_vulkan::VulkanRenderer::~VulkanRenderer() {}

void ee_vulkan::VulkanRenderer::Init() {
    depth.emplace(*context, surface);

    LoadProvincesMaterial();

    viewProjectionUniform.emplace(*context, MAX_FRAMES_IN_FLIGHT);

    LoadProvinces();

    isDrawing = true;
}

void ee_vulkan::VulkanRenderer::Shutdown() {
    context->device.handle.waitIdle();
}

void ee_vulkan::VulkanRenderer::Clear() {}

void ee_vulkan::VulkanRenderer::DrawMap() {
    if (!isDrawing) return;

    syncManager->WaitInFlightFence(currentFrame);

    std::uint32_t imageIndex = 0;
    switch (vk::Result result = syncManager->AcquireNextImage(*swapchain, currentFrame, imageIndex)) {
        case vk::Result::eSuccess:
            break;

        case vk::Result::eErrorOutOfDateKHR:
        case vk::Result::eSuboptimalKHR:
            LOG(WARNING, "Swapchain needs to be recreated");
            RecreateSwapchain();
            return;

        default:
            LOG(ERROR, "Failed to get next image index. Code: %d", result);
            break;
    }

    UpdateData(currentFrame);

    syncManager->WaitImageInFlight(currentFrame, imageIndex);

    const vulkan_render::Command &renderCommand = renderCommands->graphicsBuffers[currentFrame];
    renderCommand.handle.reset();
    renderCommand.Begin();

    std::array clears = {DrawConfig::CLEAR_COLOR, DrawConfig::CLEAR_DEPTH};
    renderCommand.handle.beginRenderPass({
        .renderPass = provincesPipeline->renderPass,
        .framebuffer = provincesPipeline->framebuffers[imageIndex],
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = surface.extent
        },
        .clearValueCount = static_cast<std::uint32_t>(clears.size()),
        .pClearValues = clears.data()
    }, vk::SubpassContents::eInline);

    renderCommand.handle.bindPipeline(vk::PipelineBindPoint::eGraphics, provincesPipeline->handle);
    renderCommand.handle.setViewport(0, {provincesPipeline->viewport});
    renderCommand.handle.setScissor(0, {provincesPipeline->scissors});

    renderCommand.handle.bindVertexBuffers(
        0,
        {provinces.vertices->handle},
        {0}
    );
    renderCommand.handle.bindIndexBuffer(
        provinces.indices->handle,
        0,
        vk::IndexType::eUint32
    );

    renderCommand.handle.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        provincesPipeline->layout,
        0,
        {provincesPipeline->descriptorSets[currentFrame]},
        {}
    );

    renderCommand.handle.drawIndexedIndirect(
        provinces.indirectCommands->handle,
        0,
        provinces.indirectCommands->content.size(),
        provinces.indirectCommands->elementSize
    );

    renderCommand.handle.endRenderPass();
    renderCommand.handle.end();

    context->device.handle.resetFences({syncManager->inFlightFences[currentFrame]});

    vk::CommandBufferSubmitInfo cmdSubmitInfo {
        .commandBuffer = renderCommand.handle
    };
    vk::SemaphoreSubmitInfo waitSemaphoreInfo {
        .semaphore = syncManager->imageAvailable[currentFrame],
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
    };
    vk::SemaphoreSubmitInfo signalSemaphoreInfo {
        .semaphore = syncManager->renderFinished[currentFrame],
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
    };
    const vk::SubmitInfo2 submitInfo {
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdSubmitInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signalSemaphoreInfo,
    };
    context->device.graphics.submit2({submitInfo}, syncManager->inFlightFences[currentFrame]);

    vk::SwapchainKHR swapchains[] = {swapchain->handle};
    switch (
        vk::Result result = context->device.present.presentKHR({
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &signalSemaphoreInfo.semaphore,
            .swapchainCount = 1,
            .pSwapchains = swapchains,
            .pImageIndices = &imageIndex
        })
    ) {
        case vk::Result::eSuccess:
            break;

        case vk::Result::eErrorOutOfDateKHR:
        case vk::Result::eSuboptimalKHR:
            LOG(WARNING, "Swapchain needs to be recreated");
            RecreateSwapchain();
            return;

        default:
            LOG(ERROR, "Failed to get next image index. Code: %d", result);
            break;
    }

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void ee_vulkan::VulkanRenderer::DrawUI() {}

void ee_vulkan::VulkanRenderer::ResizeWindow(const int width, const int height) {
    IRenderer::ResizeWindow(width, height);
    surface.extent = {
        .width = static_cast<std::uint32_t>(screenWidth),
        .height = static_cast<std::uint32_t>(screenHeight)
    };
}

void ee_vulkan::VulkanRenderer::ApplyCamera() {
    viewProjection[1][1] *= -1;
    viewProjectionInversed[1][1] *= -1;
}

void ee_vulkan::VulkanRenderer::UpdateProvinces(const std::size_t offset, const std::vector<ee_engine::ProvinceSSBO> &data) {
    const std::size_t elementOffset = offset / provinces.storage->elementSize;
    std::ranges::copy(data, provinces.storage->content.data() + elementOffset);
    renderCommands->SubmitStagingBuffer(*provinces.storage);
}

void ee_vulkan::VulkanRenderer::RecreateSwapchain() {
    // We should not render with minimized window
    if (screenWidth == 0 || screenHeight == 0)
    {
        isDrawing = false;
        return;
    }
    isDrawing = true;

    context->device.handle.waitIdle();
    swapchain = vulkan_render::ResourcesFactory::CreateSwapchain(*context, surface);
    depth.emplace(*context, surface);
    LoadProvincesMaterial();
}

void ee_vulkan::VulkanRenderer::LoadProvincesMaterial() {
    constexpr vulkan_render::ShaderBindings shaderBindings {
        .vertexBinding = {
            .binding = 0,
            .stride = sizeof(glm::vec2),
            .inputRate = vk::VertexInputRate::eVertex
        },
        .vertexAttributesCount = 1,
        .vertexAttributes = {
            .location = 0,
            .binding = 0,
            .format = vulkan_render::ShaderAttributeFormat::VEC2,
            .offset = 0
        }
    };

    const vk::DescriptorSetLayoutBinding viewProjection {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .pImmutableSamplers = nullptr
    };
    const vk::DescriptorSetLayoutBinding provincesSSBO {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::Flags(vk::ShaderStageFlagBits::eVertex) | vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
    };

    const vk::PushConstantRange model {
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(glm::mat4)
    };

    vulkan_render::PipelineConfig provincesMaterial = vulkan_render::Pipeline::MakeConfig(
        surface,
        shaderBindings,
        {viewProjection, provincesSSBO},
        model
    );
    vulkan_render::ShaderProgram provincesShader {
        .vertex = {reinterpret_cast<std::byte*>(shaders.vertProvinces->content), shaders.vertProvinces->size},
        .fragment = {reinterpret_cast<std::byte*>(shaders.fragProvinces->content), shaders.fragProvinces->size}
    };

    provincesPipeline.emplace(
        *context,
        surface,
        *swapchain,
        *depth,
        provincesMaterial,
        provincesShader,
        MAX_FRAMES_IN_FLIGHT
    );
}

void ee_vulkan::VulkanRenderer::LoadProvinces() {
    std::vector<ee_engine::ProvinceSSBO> provincesData;
    std::vector<glm::vec2> verticesData;
    std::vector<std::uint32_t> indicesData;
    std::vector<vk::DrawIndexedIndirectCommand> indirectCommandsData;

    std::size_t regionOffset = 0;
    std::uint32_t indicesOffset = 0;
    std::int32_t verticesOffset = 0;
    for (auto& [region, provs] : mapMesh->regions) {
        for (auto&[vertices, indices] : provs) {
            indirectCommandsData.push_back({
                .indexCount = static_cast<std::uint32_t>(indices.size()),
                .instanceCount = 1,
                .firstIndex = indicesOffset,
                .vertexOffset = verticesOffset,
                .firstInstance = static_cast<std::uint32_t>(provincesData.size())
            });

            verticesData.append_range(vertices);
            indicesData.append_range(indices);

            ee_engine::ProvinceSSBO data{
                .regionId = region,
                .color = 0xFF0000
            };
            provincesData.push_back(data);

            indicesOffset += indices.size();
            verticesOffset += vertices.size();
        }

        regionsMapping[region] = {.offset = regionOffset, .provincesCount = provs.size()};
        regionOffset += provs.size() * sizeof(ee_engine::ProvinceSSBO);
    }

    provinces.vertices.emplace(*context, verticesData.size());
    provinces.indices.emplace(*context, indicesData.size());
    provinces.storage.emplace(*context, provincesData.size());
    provinces.indirectCommands.emplace(*context, indirectCommandsData.size());

    std::ranges::copy(verticesData, provinces.vertices->content.data());
    std::ranges::copy(indicesData, provinces.indices->content.data());
    std::ranges::copy(provincesData, provinces.storage->content.data());
    std::ranges::copy(indirectCommandsData, provinces.indirectCommands->content.data());

    renderCommands->SubmitStagingBuffer(*provinces.vertices);
    renderCommands->SubmitStagingBuffer(*provinces.indices);
    renderCommands->SubmitStagingBuffer(*provinces.storage);
    renderCommands->SubmitStagingBuffer(*provinces.indirectCommands);
}

void ee_vulkan::VulkanRenderer::UpdateData(const std::uint32_t frame) {
    std::span<glm::mat4> &vpBuffer = viewProjectionUniform->buffers[frame].content;
    vpBuffer[0] = viewProjection;

    provincesPipeline->UpdateDescriptors({
        {
            .info = vk::DescriptorBufferInfo{
                .buffer = viewProjectionUniform->buffers[frame].handle,
                .offset = 0,
                .range = viewProjectionUniform->buffers[frame].memorySize
            },
            .bindingPosition = 0,
            .type = vk::DescriptorType::eUniformBuffer
        },
        {
            .info = vk::DescriptorBufferInfo{
                .buffer = provinces.storage->handle,
                .offset = 0,
                .range = provinces.storage->memorySize
            },
            .bindingPosition = 1,
            .type = vk::DescriptorType::eStorageBuffer
        }
    }, frame);
}
