#include "pipeline.h"

#include "vk_helper.h"

pipeline::pipeline(const engine_vk &engine) : engine(engine) {
	this->pdssci = {
		{},
		VK_FALSE,
		VK_FALSE,
		vk::CompareOp::eLess,
		VK_FALSE,
		VK_FALSE,
		{},
		{},
		0.0f,
		1.0f
	};

	this->prsci.lineWidth = 1.0f;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment {};
	colorBlendAttachment.colorWriteMask = vk::FlagTraits<vk::ColorComponentFlagBits>::allFlags;
	this->colorBlendAttachements.emplace_back(colorBlendAttachment);

	this->pcbsci = {
		{},
		{},
		{},
		static_cast<std::uint32_t>(this->colorBlendAttachements.size()), this->colorBlendAttachements.data(),
		{}
	};

	this->gpci.stageCount = static_cast<std::uint32_t>(this->shaderStages.size());
	this->gpci.pStages = this->shaderStages.data();
	this->gpci.pVertexInputState = &this->pvisci;
	this->gpci.pInputAssemblyState = &this->piasci;
	this->gpci.pTessellationState = &this->ptsci;
	this->gpci.pViewportState = &this->pvsci;
	this->gpci.pRasterizationState = &this->prsci;
	this->gpci.pMultisampleState = &this->pmsci;
	this->gpci.pDepthStencilState = &this->pdssci;
	this->gpci.pColorBlendState = &this->pcbsci;
	this->gpci.pDynamicState = nullptr;
	this->gpci.subpass = 0;
	this->gpci.basePipelineHandle = nullptr;
	this->gpci.basePipelineIndex = -1;
}

void pipeline::addShader(const vk::ShaderStageFlagBits& type, const std::string &filename) noexcept {
	auto shader = this->engine.createShaderModule(filename);
	const vk::SpecializationInfo *spPointer = nullptr;

	vk::PipelineShaderStageCreateInfo pssci {
		{},
		type,
		shader.get(),
		"main",
		spPointer
	};

	this->shaderModules.emplace_back(type, std::move(shader));
	this->shaderStages.emplace_back(pssci);
}

void pipeline::finalize(const renderpass& renderpass, const swapchain& swapchain) {
	this->gpci.renderPass = renderpass.renderPass.get();
	this->gpci.subpass = 0;

	this->gpci.stageCount = static_cast<std::uint32_t>(this->shaderStages.size());
	this->gpci.pStages = this->shaderStages.data();

	this->plci.setLayoutCount = 0;//static_cast<std::uint32_t>(this->descriptorLayoutSets.size());
	this->plci.pSetLayouts = &this->descriptorLayout.get();//this->descriptorLayoutSets.data();

	this->pipeLineLayout = this->engine.logicalDevice->createPipelineLayoutUnique(this->plci);

	this->gpci.layout = this->pipeLineLayout.get();

	this->pipeLine = this->engine.logicalDevice->createGraphicsPipelineUnique(vk::PipelineCache(), this->gpci).value;
}

void pipeline::setViewPortScissor(const vk::Extent2D &size) {
	this->viewPorts = { {0.0f, 0.0f, static_cast<float>(size.width), static_cast<float>(size.height), 0.0f, 1.0f} };
	this->scissors = { vk::Rect2D{vk::Offset2D{0, 0}, size} };

	this->pvsci.viewportCount = static_cast<std::uint32_t>(this->viewPorts.size());
	this->pvsci.pViewports = this->viewPorts.data();
	this->pvsci.scissorCount = static_cast<std::uint32_t>(this->scissors.size());
	this->pvsci.pScissors = this->scissors.data();
}

void pipeline::setDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& descriptorSetLayoutBindings) {
	const vk::DescriptorSetLayoutCreateInfo dslci {
		{},
		static_cast<std::uint32_t>(descriptorSetLayoutBindings.size()),
		descriptorSetLayoutBindings.data()
	};
	this->descriptorLayout = this->engine.logicalDevice->createDescriptorSetLayoutUnique(dslci);
//	this->descriptorLayoutSets.emplace_back(*descriptorSet);
//	this->descriptorLayoutSetsBuffer.emplace_back(std::move(descriptorSet));
}

void pipeline::createDescriptorSetPool(const std::vector<vk::DescriptorPoolSize>& poolSizes, std::uint32_t maxSets) {
	const vk::DescriptorPoolCreateInfo dpci {
		{},
		maxSets,
		static_cast<std::uint32_t>(poolSizes.size()),
		poolSizes.data()
	};
	this->descriptorPool = this->engine.logicalDevice->createDescriptorPoolUnique(dpci);
}

void pipeline::bind(const vk::UniqueCommandBuffer& buffer, vk::PipelineBindPoint pipelineBindPoint) {
	buffer->bindPipeline(pipelineBindPoint, this->pipeLine.get());
}

void pipeline::bindDescriptorSets(const vk::UniqueCommandBuffer& buffer, vk::PipelineBindPoint pipelineBindPoint, std::uint32_t firstSet, std::uint32_t descriptorSetCount, const vk::DescriptorSet* pDescriptorSets, std::uint32_t dynamicOffsetCount, const std::uint32_t* pDynamicOffsets) {
	buffer->bindDescriptorSets(pipelineBindPoint, this->pipeLineLayout.get(), firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

std::vector<vk::DescriptorSet> pipeline::getSets(std::uint32_t descriptorCount) {
	std::vector<vk::DescriptorSetLayout> layouts(descriptorCount, this->descriptorLayout.get());

	const vk::DescriptorSetAllocateInfo dsai {
			this->descriptorPool.get(),
			static_cast<std::uint32_t>(layouts.size()), layouts.data()
	};

	return this->engine.logicalDevice->allocateDescriptorSets(dsai);
}
