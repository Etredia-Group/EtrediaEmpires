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

PipelineFactory::PipelineFactory(
    const AssetMaterial &baseMaterial,
    const VkRenderPass &baseRenderPass,
    VkDevice &device,
    VkExtent2D &surfaceExtent
) :
    device(device), surfaceExtent(surfaceExtent)
{
    basePipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    basePipelineConfig.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);

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
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    LOG(INFO, "Creating base material pipeline...");
    Create(baseMaterial, baseRenderPass, PIPELINE_CREATE_NEW);
    LOG(INFO, "Base material pipeline created");
}

Pipeline& PipelineFactory::Get(const AssetMaterial &material)
{
    if (pipelines.contains(&material))
        return pipelines[&material];
    else return basePipeline;
}

void PipelineFactory::Create(const AssetMaterial &material, const VkRenderPass &renderPass, const uint32_t &flags)
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
    
    // Configure shaders
    AssetHandle shaderHandle;
    AssetShader *shader;
    VkShaderModuleCreateInfo shaderModuleCreateConfig;
    std::vector<VkPipelineShaderStageCreateInfo> stagesConfigs;

    stagesConfigs.reserve(3);

    // Vertex
    VkShaderModule vertexModule;
    shaderHandle = Engine::assetManager->Find(material.vertexShaderAssetID);
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
    shaderHandle = Engine::assetManager->Find(material.fragmentShaderAssetID);
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
    config->pipelineConfig.renderPass = renderPass;
    config->pipelineConfig.subpass = material.renderStage;

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
        pipelines[&material] = newPipeline;

    result->viewport = config->defaultViewport;
    result->scissors = config->defaultScissors;

    vkDestroyShaderModule(device, vertexModule, nullptr);
    vkDestroyShaderModule(device, fragmentModule, nullptr);
}

PipelineFactory::~PipelineFactory()
{
    vkDestroyPipeline(device, basePipeline.instance, nullptr);
    vkDestroyPipelineLayout(device, basePipeline.layout, nullptr);
    for (auto &item : pipelines)
    {
        vkDestroyPipeline(device, item.second.instance, nullptr);
        vkDestroyPipelineLayout(device, item.second.layout, nullptr);
    }
}