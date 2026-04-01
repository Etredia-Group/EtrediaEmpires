module vulkan_render;
import utils;

vulkan_render::Pipeline::Pipeline(
    const Context &context,
    const Surface &surface,
    const Swapchain &swapchain,
    const Texture<TextureMode::DEPTH> &depth,
    const PipelineConfig &config,
    const ShaderProgram &shaderProgram,
    const std::uint32_t maxFramesInFlight
) : context(context),
    surface(surface),
    swapchain(swapchain),
    maxFramesInFlight(maxFramesInFlight),
    descriptorsLayout(MakeDescriptorsLayout(config)),
    layout(MakeLayout(config)),
    renderPass(MakerRenderPass()),
    handle(MakeInstance(config, shaderProgram)),
    viewport(config.defaultViewport),
    scissors(config.defaultScissors),
    descriptorPool(MakeDescriptorPool(config.descriptors)),
    descriptorSets(MakeDescriptorSets()) {
    MakeFramebuffers(depth);
    LOG(INFO, "Pipeline created");
}

void vulkan_render::Pipeline::UpdateDescriptors(
    const std::vector<DescriptorInfo> &descriptors,
    const std::uint32_t frame
) {
    std::vector<vk::WriteDescriptorSet> writes;
    writes.reserve(descriptors.size());

    for (const auto &[info, bindingPosition, type] : descriptors) {
        vk::WriteDescriptorSet write {
            .dstSet = descriptorSets[frame],
            .dstBinding = bindingPosition,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = type,
        };

        switch (type) {
            case vk::DescriptorType::eStorageBuffer:
            case vk::DescriptorType::eUniformBuffer:
                write.pBufferInfo = &std::get<vk::DescriptorBufferInfo>(info);
                break;
            case vk::DescriptorType::eCombinedImageSampler:
                write.pImageInfo = &std::get<vk::DescriptorImageInfo>(info);
                break;

            default:
                break;
        }

        writes.push_back(write);
    }

    context.device.handle.updateDescriptorSets(writes, 0);
}

vulkan_render::PipelineConfig vulkan_render::Pipeline::MakeConfig(
    const Surface &surface,
    const ShaderBindings &bindings,
    const DescriptorBindings &descriptors,
    const vk::PushConstantRange &pushConstants
) {
    PipelineConfig config;

    config.dynamicStates.push_back(vk::DynamicState::eViewport);
    config.dynamicStates.push_back(vk::DynamicState::eScissor);

    config.colorBlendAttachmentStates.push_back({
        .blendEnable = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::Flags(vk::ColorComponentFlagBits::eR) | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
    });

    config.descriptors = descriptors;

    config.vertexInputInfo = {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindings.vertexBinding,
        .vertexAttributeDescriptionCount = bindings.vertexAttributesCount,
        .pVertexAttributeDescriptions = &bindings.vertexAttributes
    };

    config.inputAssemblyInfo = {
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False
    };

    config.rasterizerInfo = {
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode =  vk::CullModeFlagBits::eNone, //TODO: BACK_BIT
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .lineWidth = 1.0f,
    };

    config.multisamplingInfo = {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
    };

    config.depthStencilInfo = {
        .depthTestEnable = vk::False,//TODO
        .depthWriteEnable = vk::False,//TODO
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable = vk::False,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    config.dynamicStateInfo = {
        .dynamicStateCount = static_cast<std::uint32_t>(config.dynamicStates.size()),
        .pDynamicStates = config.dynamicStates.data()
    };

    config.descriptorsInfo = {
        .bindingCount = static_cast<std::uint32_t>(config.descriptors.size()),
        .pBindings = config.descriptors.data()
    };

    config.pushConstantRange = pushConstants;

    config.defaultViewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(surface.extent.width),
        .height = static_cast<float>(surface.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    config.defaultScissors = {
        .offset = {.x = 0, .y = 0},
        .extent = surface.extent
    };

    config.viewportStateInfo = {
        .viewportCount = 1,
        .pViewports = &config.defaultViewport,
        .scissorCount = 1,
        .pScissors = &config.defaultScissors
    };

    config.colorBlendInfo = {
        .logicOpEnable = vk::False,
        .attachmentCount = static_cast<std::uint32_t>(config.colorBlendAttachmentStates.size()),
        .pAttachments = config.colorBlendAttachmentStates.data(),
    };

    return config;
}

vk::raii::DescriptorSetLayout vulkan_render::Pipeline::MakeDescriptorsLayout(const PipelineConfig &config) const {
    return context.device.handle.createDescriptorSetLayout(config.descriptorsInfo);
}

vk::raii::PipelineLayout vulkan_render::Pipeline::MakeLayout(const PipelineConfig &config) const {
    const vk::PipelineLayoutCreateInfo layoutInfo {
        .setLayoutCount = 1,
        .pSetLayouts = &*descriptorsLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &config.pushConstantRange
    };

    return context.device.handle.createPipelineLayout(layoutInfo);
}

vk::raii::Pipeline vulkan_render::Pipeline::MakeInstance(
    const PipelineConfig &config,
    const ShaderProgram &shaderProgram
) const {
    // Configure shaders
    std::vector<vk::PipelineShaderStageCreateInfo> stagesInfo;
    stagesInfo.reserve(3);

    // Vertex
    vk::ShaderModuleCreateInfo shaderModuleCreateInfo = {
        .codeSize = shaderProgram.vertex.size(),
        .pCode = reinterpret_cast<const std::uint32_t*>(shaderProgram.vertex.data())
    };
    const vk::raii::ShaderModule vertexModule = context.device.handle.createShaderModule(shaderModuleCreateInfo);
    stagesInfo.push_back({
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = vertexModule,
        .pName = shaderProgram.vertexMainFun,
        .pSpecializationInfo = nullptr
    });

    // Fragment
    shaderModuleCreateInfo = {
        .codeSize = shaderProgram.fragment.size(),
        .pCode = reinterpret_cast<const std::uint32_t*>(shaderProgram.fragment.data())
    };
    const vk::raii::ShaderModule fragmentModule = context.device.handle.createShaderModule(shaderModuleCreateInfo);
    stagesInfo.push_back({
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = fragmentModule,
        .pName = shaderProgram.fragmentMainFun,
        .pSpecializationInfo = nullptr
    });

    //

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineInfo(
        {
            .stageCount = static_cast<std::uint32_t>(stagesInfo.size()),
            .pStages = stagesInfo.data(),
            .pVertexInputState = &config.vertexInputInfo,
            .pInputAssemblyState = &config.inputAssemblyInfo,
            .pViewportState = &config.viewportStateInfo,
            .pRasterizationState = &config.rasterizerInfo,
            .pMultisampleState = &config.multisamplingInfo,
            .pDepthStencilState = &config.depthStencilInfo,
            .pColorBlendState = &config.colorBlendInfo,
            .pDynamicState = &config.dynamicStateInfo,
            .layout = layout,
            .renderPass = renderPass,
            .basePipelineHandle = nullptr,
            .basePipelineIndex = -1,
        },
        {
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &surface.format.format,
            .depthAttachmentFormat = vk::Format::eD32SfloatS8Uint
        }
    );

    return context.device.handle.createGraphicsPipeline(nullptr, pipelineInfo.get());
}

vk::raii::DescriptorPool vulkan_render::Pipeline::MakeDescriptorPool(const DescriptorBindings &descriptors) {
    descriptorCounts.clear();
    for (const vk::DescriptorSetLayoutBinding &binding : descriptors) {
        descriptorCounts[binding.descriptorType] += binding.descriptorCount;
    }

    std::vector<vk::DescriptorPoolSize> poolSizes;
    poolSizes.reserve(descriptorCounts.size());
    for (const auto &[type, count] : descriptorCounts) {
        poolSizes.push_back({
            .type = type,
            .descriptorCount = count * maxFramesInFlight
        });
    }

    return context.device.handle.createDescriptorPool({
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = static_cast<std::uint32_t>(maxFramesInFlight),
        .poolSizeCount = static_cast<std::uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    });
}

vk::raii::DescriptorSets vulkan_render::Pipeline::MakeDescriptorSets() const {
    std::vector layouts(maxFramesInFlight, *descriptorsLayout);

    return vk::raii::DescriptorSets(
        context.device.handle,
        {
            .descriptorPool = descriptorPool,
            .descriptorSetCount = maxFramesInFlight,
            .pSetLayouts = layouts.data()
        }
    );
}

vk::raii::RenderPass vulkan_render::Pipeline::MakerRenderPass() const {
    const vk::AttachmentDescription colorAttachmentDescription {
            .format = surface.format.format,
            .samples = vk::SampleCountFlagBits::e1, // No multisampling
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::ePresentSrcKHR
    };

    vk::AttachmentReference colorAttachmentRef {
            .attachment = 0,
            .layout = vk::ImageLayout::eColorAttachmentOptimal
    };

    const vk::AttachmentDescription depthAttachmentDescription {
        .format = vk::Format::eD32SfloatS8Uint,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare ,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };

    vk::AttachmentReference depthAttachmentRef {
        .attachment = 1,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    vk::SubpassDescription subpass {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    vk::SubpassDependency dependency {
        .srcSubpass = vk::SubpassExternal,
        .dstSubpass = 0,
        .srcStageMask = vk::Flags(vk::PipelineStageFlagBits::eColorAttachmentOutput) | vk::PipelineStageFlagBits::eLateFragmentTests,
        .dstStageMask = vk::Flags(vk::PipelineStageFlagBits::eColorAttachmentOutput) | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        .dstAccessMask = vk::Flags(vk::AccessFlagBits::eColorAttachmentWrite) | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
    };

    std::array attachments = {colorAttachmentDescription, depthAttachmentDescription};
    const vk::RenderPassCreateInfo baseRenderPassInfo {
        .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    return context.device.handle.createRenderPass(baseRenderPassInfo);
}

void vulkan_render::Pipeline::MakeFramebuffers(const Texture<TextureMode::DEPTH> &depth) {
    framebuffers.reserve(swapchain.frameViews.size());
    for (const vk::raii::ImageView &frameView : swapchain.frameViews)
    {
        std::array attachments = {*frameView, *depth.view};

        vk::FramebufferCreateInfo frameBufferInfo {
            .renderPass = renderPass,
            .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = surface.extent.width,
            .height = surface.extent.height,
            .layers = 1
        };

        framebuffers.emplace_back(context.device.handle.createFramebuffer(frameBufferInfo));
    }
}
