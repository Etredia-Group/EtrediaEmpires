module vulkan_render;
import utils;

vulkan_render::Command::Command(const Context &context, vk::raii::CommandBuffer &&buffer) :
    context(context),
    handle(std::move(buffer)) {}

vulkan_render::Command::Command(const Context &context, const vk::raii::CommandPool &pool) :
    context(context),
    handle(std::move(
        context.device.handle.allocateCommandBuffers({
            .commandPool = pool,
            .level = vk::CommandBufferLevel::ePrimary, //TODO: reuse some commands using SECONDARY
            .commandBufferCount = 1
        }).front()
    )) {}

void vulkan_render::Command::Begin() const {
    handle.begin({
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    });
}

void vulkan_render::Command::End() {
    handle.end();
    isEnd = true;
}

void vulkan_render::Command::Submit(const vk::raii::Queue &queue) const {
    if (!isEnd) {
        handle.end();
    }

    vk::CommandBufferSubmitInfo cmdSubmitInfo {
        .commandBuffer = handle
    };

    const vk::SubmitInfo2 submitInfo {
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdSubmitInfo,

    };

    const vk::raii::Fence fence = context.device.handle.createFence({});
    queue.submit2({submitInfo}, fence);
    if (
        vk::Result result = context.device.handle.waitForFences({fence}, true, std::numeric_limits<std::uint64_t>::max());
        result != vk::Result::eSuccess
    ) {
        LOG(WARNING, "Command buffer submit timeout. Code: %d", result);
    }
}

void vulkan_render::Command::Reset() const {
    handle.reset();
}

vulkan_render::RenderCommands::RenderCommands(const Context &context, const std::uint32_t maxFramesInFlight) :
    graphicsPool(MakePool(context, context.physicalDevice.queueFamilies.graphics)),
    transferPool(MakePool(context, context.physicalDevice.queueFamilies.transfer)) {

    graphicsBuffers = MakeBuffers(context, graphicsPool, maxFramesInFlight);
    transferBuffers = MakeBuffers(context, transferPool, maxFramesInFlight);

    LOG(INFO, "Command buffers initialized.");
}

void vulkan_render::RenderCommands::SubmitStagingBuffer(const BaseBuffer<BufferMode::STAGING_ACCESS> &buffer) const {
    const Command cmd(buffer.context, transferPool);
    cmd.Begin();

    vk::BufferCopy2 copyRegion {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = buffer.memorySize
    };

    cmd.handle.copyBuffer2({
        .srcBuffer = buffer.stagingHandle,
        .dstBuffer = buffer.handle,
        .regionCount = 1,
        .pRegions = &copyRegion
    });

    cmd.Submit(buffer.context.device.transfer);
}

vk::raii::CommandPool vulkan_render::RenderCommands::MakePool(const Context &context, const std::uint32_t queueFamily) {
    return context.device.handle.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamily,
    });
}

std::vector<vulkan_render::Command> vulkan_render::RenderCommands::MakeBuffers(
    const Context &context,
    const vk::raii::CommandPool &pool,
    const std::uint32_t size
) {
    std::vector<vk::raii::CommandBuffer> buffers = context.device.handle.allocateCommandBuffers({
        .commandPool = pool,
        .level = vk::CommandBufferLevel::ePrimary, //TODO: reuse some commands using SECONDARY
        .commandBufferCount = size
    });

    std::vector<Command> result;
    result.reserve(buffers.size());
    for (vk::raii::CommandBuffer &buffer : buffers) {
        result.emplace_back(context, std::move(buffer));
    }

    return result;
}

vulkan_render::UserCommands::UserCommands(const Context &context, const std::uint32_t maxFramesInFlight) :
    pool(MakePool(context, context.physicalDevice.queueFamilies.graphics)) {
    buffers = MakeBuffers(context, pool, maxFramesInFlight);
    LOG(INFO, "Command buffers initialized.");
}

vk::raii::CommandPool vulkan_render::UserCommands::MakePool(const Context &context, const std::uint32_t queueFamily) {
    return context.device.handle.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamily,
    });
}

std::vector<vulkan_render::Command> vulkan_render::UserCommands::MakeBuffers(
    const Context &context,
    const vk::raii::CommandPool &pool,
    const std::uint32_t size
) {
    std::vector<vk::raii::CommandBuffer> buffers = context.device.handle.allocateCommandBuffers({
        .commandPool = pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = size
    });

    std::vector<Command> result;
    result.reserve(buffers.size());
    for (vk::raii::CommandBuffer &buffer : buffers) {
        result.emplace_back(context, std::move(buffer));
    }

    return result;
}

vulkan_render::SyncManager::SyncManager(
    const Context &context,
    const std::uint32_t maxFramesInFlight,
    const std::uint64_t timeout
) :
    context(context), TIMEOUT(timeout) {
    imageAvailable.reserve(maxFramesInFlight);
    renderFinished.reserve(maxFramesInFlight);
    inFlightFences.reserve(maxFramesInFlight);
    imagesInFlight.resize(maxFramesInFlight, nullptr);

    for (std::uint32_t i = 0; i < maxFramesInFlight; i++) {
        imageAvailable.push_back(context.device.handle.createSemaphore({}));
        renderFinished.push_back(context.device.handle.createSemaphore({}));
        inFlightFences.push_back(context.device.handle.createFence({.flags = vk::FenceCreateFlagBits::eSignaled}));
    }

    LOG(INFO, "Sync objects initialized.");
}

void vulkan_render::SyncManager::WaitInFlightFence(const std::size_t currentFrame) {
    if (
        vk::Result result = context.device.handle.waitForFences({inFlightFences[currentFrame]}, vk::True, TIMEOUT);
        result != vk::Result::eSuccess
    )
        LOG(ERROR, "Waiting semaphore timeout. Code: %d", result);
}
void vulkan_render::SyncManager::WaitImageInFlight(const std::size_t currentFrame, const std::uint32_t imageIndex) {
    if (imagesInFlight[imageIndex] == nullptr) return;
    if (
        vk::Result result = context.device.handle.waitForFences({imagesInFlight[imageIndex]}, vk::True, TIMEOUT);
        result != vk::Result::eSuccess
    )
        LOG(ERROR, "Waiting semaphore timeout. Code: %d", result);
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];
}

vk::Result vulkan_render::SyncManager::AcquireNextImage(
    const Swapchain &swapchain,
    const std::size_t currentFrame,
    std::uint32_t &imageIndex
) const {
    auto [result, newImageIndex] = context.device.handle.acquireNextImage2KHR({
        .swapchain = swapchain.handle,
        .timeout = TIMEOUT,
        .semaphore = imageAvailable[currentFrame],
        .deviceMask = 1
    });

    imageIndex = newImageIndex;
    return result;
}

vulkan_render::FrameContext::FrameContext(
    const RenderPass &renderPass,
    const Surface &surface,
    const RenderCommands &renderCommands,
    const std::uint32_t imageIndex,
    const std::uint32_t currentFrame,
    const std::span<vk::ClearValue> clearValues
) {
    this->imageIndex = imageIndex;
    this->currentFrame = currentFrame;

    renderCommand = &renderCommands.graphicsBuffers[currentFrame];
    renderCommand->handle.reset();
    renderCommand->Begin();

    renderCommand->handle.beginRenderPass({
        .renderPass = renderPass.handle,
        .framebuffer = renderPass.framebuffers[imageIndex],
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = surface.extent
        },
        .clearValueCount = static_cast<std::uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data()
    }, vk::SubpassContents::eInline);
}

void vulkan_render::FrameContext::Complete() const {
    renderCommand->handle.endRenderPass();
    renderCommand->handle.end();
}
