#pragma once
#include "AssetData.hpp"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <unordered_map>

#define PIPELINE_CREATE_NEW 0
#define PIPELINE_CREATE_DERIVED (1 << 0)

struct PipelineBinding
{
	const AssetMaterial *baseMaterial;

	const VkRenderPass *baseRenderPass;
	const VkDevice *device;
	const VkExtent2D *surfaceExtent;

	const VkVertexInputBindingDescription *vertexBindingConfig;
	uint32_t vertexAttributesCount;
	const VkVertexInputAttributeDescription *vertexAttributesConfig;
};

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
	
	std::vector<VkDescriptorSetLayoutBinding> uniforms;
	int vertexBindingsCount, fragmentBindingsCount;
	VkDescriptorSetLayoutCreateInfo uniformsConfig;

	VkGraphicsPipelineCreateInfo pipelineConfig;

	PipelineConfig Copy() const;
};

struct Pipeline
{
	VkPipeline instance;
	VkPipelineLayout layout;
	VkDescriptorSetLayout uniformsLayout;
	VkViewport viewport;
	VkRect2D scissors;
};

class PipelineFactory
{
public:
	PipelineFactory(const PipelineBinding &binding);
	~PipelineFactory();

	Pipeline& Get(const AssetMaterial &material);
	void Create(const AssetMaterial &material, const VkRenderPass &renderPass, const uint32_t &flags);

private:
	PipelineBinding binding;
	PipelineConfig basePipelineConfig;

	Pipeline basePipeline;
	std::unordered_map<const AssetMaterial*, Pipeline> pipelines;

	const VkDevice &device;
	const VkExtent2D &surfaceExtent;

	const VkVertexInputBindingDescription &vertexBindingConfig;

};