#include "PipelineFactory.hpp"
#include "Engine.hpp"
#include "Logger.h"

PipelineConfig PipelineConfig::Copy() const
{
    PipelineConfig copy = *this;
    
    copy.viewportStateConfig.pViewports = &copy.defaultViewport;
    copy.viewportStateConfig.pScissors = &copy.defaultScissors;
    copy.dynamicStateConfig.pDynamicStates = copy.dynamicStates.data();
    copy.colorBlendConfig.pAttachments = copy.colorBlendAttachmentConfigs.data();
    copy.renderPassConfig.pAttachments = copy.colorAttachmentConfigs.data();
    copy.renderPassConfig.pSubpasses = copy.subpassConfigs.data();
    for (VkSubpassDescription &config : copy.subpassConfigs)
    {
      config.pColorAttachments = copy.colorAttachmentRefs.data();
    }

    copy.pipelineConfig.pVertexInputState = &copy.vertexInputConfig;
    copy.pipelineConfig.pInputAssemblyState = &copy.inputAssemblyConfig;
    copy.pipelineConfig.pViewportState = &copy.viewportStateConfig;
    copy.pipelineConfig.pRasterizationState = &copy.rasterizerConfig;
    copy.pipelineConfig.pMultisampleState = &copy.multisamplingConfig;
    copy.pipelineConfig.pDepthStencilState = nullptr;
    copy.pipelineConfig.pColorBlendState = &copy.colorBlendConfig;
    copy.pipelineConfig.pDynamicState = &copy.dynamicStateConfig;

    return copy;
}

PipelineFactory::PipelineFactory(const AssetMaterial &baseMaterial, VkDevice &device, VkExtent2D &surfaceExtent, VkFormat &surfaceFormat) :
    device(device), surfaceExtent(surfaceExtent), surfaceFormat(surfaceFormat)
{
    basePipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);

    basePipelineConfig.colorBlendAttachmentConfigs.push_back({
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    });

    basePipelineConfig.colorAttachmentConfigs.push_back({
		.format = surfaceFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT, // TODO: Multisampling
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	});
    basePipelineConfig.colorAttachmentRefs.push_back({
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	});

    basePipelineConfig.subpassConfigs.push_back({
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = static_cast<uint32_t>(basePipelineConfig.colorAttachmentRefs.size()),
		.pColorAttachments = basePipelineConfig.colorAttachmentRefs.data()
    });

    basePipelineConfig.vertexInputConfig = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    basePipelineConfig.inputAssemblyConfig = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    basePipelineConfig.rasterizerConfig = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    basePipelineConfig.multisamplingConfig = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };

    basePipelineConfig.depthStencilConfig = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
    };

    basePipelineConfig.dynamicStateConfig = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .dynamicStateCount = static_cast<uint32_t>(basePipelineConfig.dynamicStates.size()),
        .pDynamicStates = basePipelineConfig.dynamicStates.data()
    };

    basePipelineConfig.pipelineLayoutConfig = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .setLayoutCount = 0, //TODO: vector
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    basePipelineConfig.defaultViewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(surfaceExtent.width),
        .height = static_cast<float>(surfaceExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    
    basePipelineConfig.defaultScissors = {
        .offset = {.x = 0, .y = 0},
        .extent = surfaceExtent
    };

    basePipelineConfig.viewportStateConfig = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .pViewports = &basePipelineConfig.defaultViewport,
        .scissorCount = 1,
        .pScissors = &basePipelineConfig.defaultScissors
    };

    basePipelineConfig.colorBlendConfig = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = static_cast<uint32_t>(basePipelineConfig.colorBlendAttachmentConfigs.size()),
        .pAttachments = basePipelineConfig.colorBlendAttachmentConfigs.data(),
    };

    basePipelineConfig.renderPassConfig = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .attachmentCount = static_cast<uint32_t>(basePipelineConfig.colorAttachmentConfigs.size()),
        .pAttachments = basePipelineConfig.colorAttachmentConfigs.data(),
        .subpassCount = static_cast<uint32_t>(basePipelineConfig.subpassConfigs.size()),
        .pSubpasses = basePipelineConfig.subpassConfigs.data()
    };

    basePipelineConfig.pipelineConfig = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .pVertexInputState = &basePipelineConfig.vertexInputConfig,
        .pInputAssemblyState = &basePipelineConfig.inputAssemblyConfig,
        .pViewportState = &basePipelineConfig.viewportStateConfig,
        .pRasterizationState = &basePipelineConfig.rasterizerConfig,
        .pMultisampleState = &basePipelineConfig.multisamplingConfig,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &basePipelineConfig.colorBlendConfig,
        .pDynamicState = &basePipelineConfig.dynamicStateConfig,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    LOG(INFO, "Creating base material pipeline...");
    Create(baseMaterial, PIPELINE_CREATE_NEW);
    LOG(INFO, "Base material pipeline created");
}

void PipelineFactory::Create(const AssetMaterial &material, uint32_t flags)
{
    Pipeline newPipeline {};
    Pipeline *result = &basePipeline;
    
    PipelineConfig newConfig;
    PipelineConfig *config = &basePipelineConfig;
    
    if (flags & PIPELINE_CREATE_DERIVED)
    {
        newConfig = basePipelineConfig.Copy();
        config = &newConfig;

        result = &newPipeline;
        config->pipelineConfig.basePipelineHandle = basePipeline.instance;
    }

    VkResult vkResult = vkCreatePipelineLayout(
        device,
        &config->pipelineLayoutConfig,
        nullptr,
        &result->layout
    );
    if (vkResult != VK_SUCCESS)
        LOG(CRITICAL, "Failed to create pipeline layout");
    config->pipelineConfig.layout = result->layout;

    vkResult = vkCreateRenderPass(device, &config->renderPassConfig, nullptr, &result->renderPass);
    if (vkResult != VK_SUCCESS)
        LOG(CRITICAL, "Failed to create render pass");
    config->pipelineConfig.renderPass = result->renderPass;
    
    // Configure shaders
    AssetHandle shaderHandle;
    AssetShader *shader;
    VkShaderModuleCreateInfo shaderModuleCreateConfig;
    std::vector<VkPipelineShaderStageCreateInfo> stagesConfigs;

    stagesConfigs.reserve(3);

    // Vertex
    VkShaderModule vertexModule;
    shaderHandle = Engine::assetManager->Find(material.vertexShader);
    shader = &Engine::assetManager->Get<AssetShader>(shaderHandle);
    shaderModuleCreateConfig = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .codeSize = shader->size,
        .pCode = shader->content
    };
    vkResult = vkCreateShaderModule(device, &shaderModuleCreateConfig, nullptr, &vertexModule);
    if (vkResult != VK_SUCCESS)
        LOG(ERROR, "Failed to create vertex shader module [Asset ID: %d]. Code: %d", shaderHandle.id, vkResult);
    stagesConfigs.push_back({
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertexModule,
        .pName = "main",
        .pSpecializationInfo = nullptr
    });

    // Fragment
    VkShaderModule fragmentModule;
    shaderHandle = Engine::assetManager->Find(material.fragmentShader);
    shader = &Engine::assetManager->Get<AssetShader>(shaderHandle);
    shaderModuleCreateConfig = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .codeSize = shader->size,
        .pCode = shader->content
    };
    vkResult = vkCreateShaderModule(device, &shaderModuleCreateConfig, nullptr, &fragmentModule);
    if (vkResult != VK_SUCCESS)
        LOG(ERROR, "Failed to create fragment shader module [Asset ID: %d]. Code: %d", shaderHandle.id, vkResult);
    stagesConfigs.push_back({
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragmentModule,
        .pName = "main",
        .pSpecializationInfo = nullptr
    });

    config->pipelineConfig.stageCount = static_cast<uint32_t>(stagesConfigs.size());
    config->pipelineConfig.pStages = stagesConfigs.data();

    vkResult = vkCreateGraphicsPipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &config->pipelineConfig,
        nullptr,
        &result->instance
    );
    if (vkResult != VK_SUCCESS)
        LOG(CRITICAL, "Failed to create pipeline");

    if (flags & PIPELINE_CREATE_DERIVED)
        pipelines.push_back(newPipeline);

    vkDestroyShaderModule(device, vertexModule, nullptr);
    vkDestroyShaderModule(device, fragmentModule, nullptr);
}

PipelineFactory::~PipelineFactory()
{
    vkDestroyPipeline(device, basePipeline.instance, nullptr);
    vkDestroyPipelineLayout(device, basePipeline.layout, nullptr);
    vkDestroyRenderPass(device, basePipeline.renderPass, nullptr);
    for (Pipeline &pipeline : pipelines)
    {
        vkDestroyPipeline(device, pipeline.instance, nullptr);
        vkDestroyPipelineLayout(device, pipeline.layout, nullptr);
        vkDestroyRenderPass(device, pipeline.renderPass, nullptr);
    }
}