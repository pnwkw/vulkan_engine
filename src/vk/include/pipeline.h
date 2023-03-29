#ifndef DISPLAY_PIPELINE_H
#define DISPLAY_PIPELINE_H

#include "engine_vk.h"
#include "renderpass.h"
#include "swapchain.h"

class pipeline {
protected:
	const engine_vk& engine;

	vk::UniquePipeline pipeLine;
	vk::UniquePipelineLayout pipeLineLayout;

	std::vector<std::tuple<vk::ShaderStageFlagBits, vk::UniqueShaderModule>> shaderModules{};
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{};
	vk::UniqueDescriptorSetLayout descriptorLayout;
	vk::UniqueDescriptorPool descriptorPool;
	std::vector<vk::DescriptorSet> descriptorSets{};

	vk::PipelineVertexInputStateCreateInfo pvisci{};
	vk::PipelineInputAssemblyStateCreateInfo piasci{};
	vk::PipelineTessellationStateCreateInfo ptsci{};

	std::vector<vk::Viewport> viewPorts{};
	std::vector<vk::Rect2D> scissors{};
	vk::PipelineViewportStateCreateInfo pvsci{};
	vk::PipelineRasterizationStateCreateInfo prsci{};
	vk::PipelineMultisampleStateCreateInfo pmsci{};
	vk::PipelineDepthStencilStateCreateInfo pdssci{};

	std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachements{};
	vk::PipelineColorBlendStateCreateInfo pcbsci{};

	vk::PipelineDynamicStateCreateInfo pdsci{};

	vk::PipelineLayoutCreateInfo plci{};

	vk::GraphicsPipelineCreateInfo gpci{};

public:
	explicit pipeline(const engine_vk& engine);

	void setViewPortScissor(const vk::Extent2D &size);
	void setDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& descriptorSetLayoutBindings);

	void createDescriptorSetPool(const std::vector<vk::DescriptorPoolSize>& poolSizes, std::uint32_t maxSets);

	virtual void finalize(const renderpass& renderpass, const swapchain& swapchain);
	void bind(const vk::UniqueCommandBuffer& buffer, vk::PipelineBindPoint pipelineBindPoint);
	void bindDescriptorSets(const vk::UniqueCommandBuffer& buffer, vk::PipelineBindPoint pipelineBindPoint, std::uint32_t firstSet, std::uint32_t descriptorSetCount, const vk::DescriptorSet* pDescriptorSets, std::uint32_t dynamicOffsetCount, const std::uint32_t* pDynamicOffsets);
	std::vector<vk::DescriptorSet> getSets(std::uint32_t descriptorCount);

protected:
	void addShader(const vk::ShaderStageFlagBits& type, const std::string &filename) noexcept;

};

#endif //DISPLAY_PIPELINE_H
