#pragma once
#include "AssetData.hpp"
#include <vulkan/vulkan.hpp>
#include <vector>

#define PIPELINE_CREATE_NEW 0
#define PIPELINE_CREATE_DERIVED (1 << 0)

struct PipelineConfig
{
	VkPipelineVertexInputStateCreateInfo vertexInputConfig;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyConfig;
	VkPipelineViewportStateCreateInfo viewportStateConfig;
	VkPipelineRasterizationStateCreateInfo rasterizerConfig;
	VkPipelineMultisampleStateCreateInfo multisamplingConfig;
	VkPipelineDepthStencilStateCreateInfo depthStencilConfig;

	// SRC - new color, DST - old color
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentConfigs;
	VkPipelineColorBlendStateCreateInfo colorBlendConfig;
	VkPipelineDynamicStateCreateInfo dynamicStateConfig;
	VkPipelineLayoutCreateInfo pipelineLayoutConfig;

	VkViewport defaultViewport;
	VkRect2D defaultScissors;
	std::vector<VkDynamicState> dynamicStates;

	std::vector<VkAttachmentDescription> colorAttachmentConfigs;
	std::vector<VkAttachmentReference> colorAttachmentRefs;
	std::vector<VkSubpassDescription> subpassConfigs;
	VkRenderPassCreateInfo renderPassConfig;

	VkGraphicsPipelineCreateInfo pipelineConfig;

	PipelineConfig Copy() const;
};

struct Pipeline
{
	VkPipeline instance;
	VkPipelineLayout layout;
	VkRenderPass renderPass;
};

class PipelineFactory
{
public:
	PipelineFactory(const AssetMaterial &baseMaterial, VkDevice &device, VkExtent2D &surfaceExtent, VkFormat &surfaceFormat);
	~PipelineFactory();

	PipelineConfig &Request(const AssetMaterial &material, uint32_t flags);

private:
	PipelineConfig basePipelineConfig;

	Pipeline basePipeline;
	std::vector<Pipeline> pipelines;

	VkDevice &device;
	VkExtent2D &surfaceExtent;
	VkFormat &surfaceFormat;

	void Create(const AssetMaterial &material, uint32_t flags);
};