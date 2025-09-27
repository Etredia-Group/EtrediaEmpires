#pragma once
#include "AssetData.hpp"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <unordered_map>

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

	VkGraphicsPipelineCreateInfo pipelineConfig;

	PipelineConfig Copy() const;
};

struct Pipeline
{
	VkPipeline instance;
	VkPipelineLayout layout;
	VkViewport viewport;
	VkRect2D scissors;
};

class PipelineFactory
{
public:
	PipelineFactory(const AssetMaterial &baseMaterial, const VkRenderPass &baseRenderPass, VkDevice &device, VkExtent2D &surfaceExtent);
	~PipelineFactory();

	Pipeline& Get(const AssetMaterial &material);
	void Create(const AssetMaterial &material, const VkRenderPass &renderPass, const uint32_t &flags);

private:
	PipelineConfig basePipelineConfig;

	Pipeline basePipeline;
	std::unordered_map<const AssetMaterial*, Pipeline> pipelines;

	VkDevice &device;
	VkExtent2D &surfaceExtent;

};