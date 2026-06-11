module;

#include <glm/glm.hpp>

module vulkan_render;
import utils;

vulkan_render::Swapchain vulkan_render::ResourcesFactory::CreateSwapchain(const Context &context, Surface &surface) {
    // Clamp extent
    surface.extent.width = std::max(
        context.surfaceCapabilities.capabilities.minImageExtent.width,
        (std::min)(context.surfaceCapabilities.capabilities.maxImageExtent.width, surface.extent.width)
    );
    surface.extent.height = std::max(
        context.surfaceCapabilities.capabilities.minImageExtent.height,
        (std::min)(context.surfaceCapabilities.capabilities.maxImageExtent.height, surface.extent.height)
    );

    std::uint32_t imagesCount = context.surfaceCapabilities.capabilities.minImageCount + 1;
    // Clamp min images count
    if (context.surfaceCapabilities.capabilities.maxImageCount > 0 && imagesCount > context.surfaceCapabilities.capabilities.maxImageCount)
        imagesCount = context.surfaceCapabilities.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR swapchainCreateInfo {
        .surface = surface.handle,
        .minImageCount = imagesCount,
        .imageFormat = surface.format.format,
        .imageColorSpace = surface.format.colorSpace,
        .imageExtent = surface.extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .preTransform = context.surfaceCapabilities.capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = surface.presentMode,
        .clipped = vk::True,
        .oldSwapchain = nullptr
    };

    const std::vector queueFamiliesShare {context.physicalDevice.queueFamilies.graphics, context.physicalDevice.queueFamilies.present};
    if (context.physicalDevice.queueFamilies.graphics != context.physicalDevice.queueFamilies.present)
    {
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchainCreateInfo.queueFamilyIndexCount = queueFamiliesShare.size();
        swapchainCreateInfo.pQueueFamilyIndices = queueFamiliesShare.data();
    }
    else
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;

    Swapchain swapchain {
        .handle = context.device.handle.createSwapchainKHR(swapchainCreateInfo)
    };
    LOG(INFO, "Swapchain is created");

    SetupFrames(context, swapchain, surface);

    return swapchain;
}

void vulkan_render::ResourcesFactory::SetupFrames(const Context &context, Swapchain &swapchain, const Surface &surface) {
    swapchain.frames.clear();
    swapchain.frameViews.clear();

    swapchain.frames = swapchain.handle.getImages();
    LOG(INFO, "Frames are ready");

    swapchain.frameViews.reserve(swapchain.frames.size());
    for (size_t i = 0; i < swapchain.frames.size(); i++)
    {
        vk::ImageViewCreateInfo frameViewInfo {
            .image = swapchain.frames[i],
            .viewType = vk::ImageViewType::e2D,
            .format = surface.format.format,
            .components {
                .r = vk::ComponentSwizzle::eIdentity,
                .g = vk::ComponentSwizzle::eIdentity,
                .b = vk::ComponentSwizzle::eIdentity,
                .a = vk::ComponentSwizzle::eIdentity
            },
            .subresourceRange {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
        }
        };

        swapchain.frameViews.push_back(context.device.handle.createImageView(frameViewInfo));
    }
    LOG(INFO, "Frame views are ready.");
}

vulkan_render::RenderTexture vulkan_render::DataFactory::CreateRenderTexture(
    const Context &context,
    const RenderCommands &commands,
    const std::uint32_t width, const std::uint32_t height,
    const std::span<const std::byte> textureData
) {
    PartialTexture texture(context, width, height, vk::Format::eR8G8B8A8Srgb);
    const Buffer<
        std::byte,
        vk::BufferUsageFlagBits::eTransferSrc,
        BufferMode::STATIC
    > stagingBuffer(context, textureData);

    commands.TranslateTextureLayout(
        texture,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal
    );
    commands.SubmitTexture(stagingBuffer, texture);
    commands.TranslateTextureLayout(
        texture,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );

    texture.Init(vk::ImageAspectFlagBits::eColor);

    return std::move(texture);
}

vulkan_render::RenderTexture vulkan_render::DataFactory::CreateRenderTexture(
    const Context &context,
    const RenderCommands &commands
) {
    glm::u8vec4 pixel = {255, 255, 255, 255};
    return CreateRenderTexture(
        context,
        commands,
        1, 1,
        std::span(reinterpret_cast<std::byte*>(&pixel), 4)
    );
}
