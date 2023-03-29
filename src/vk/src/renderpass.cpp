#include "renderpass.h"

renderpass::renderpass(const engine_vk& engine, const class swapchain& swapchain) : engine(engine), swapChain(swapchain) {

	const vk::AttachmentDescription color {
		{},
		this->swapChain.format.format,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::ePresentSrcKHR
	};

	this->colorAttachements = { color };

	//	const auto index = static_cast<std::uint32_t>(this->renderPassAttachements.size());
	for(std::size_t i = 0; i < this->colorAttachements.size(); ++i) {
		this->colorAttachementRefs.emplace_back(static_cast<std::uint32_t>(i), vk::ImageLayout::eColorAttachmentOptimal);
		this->renderPassAttachements.push_back(this->colorAttachements[i]);
	}

	const vk::SubpassDescription subpass0 {
		{},
		vk::PipelineBindPoint::eGraphics,
		0,nullptr,
		1, &this->colorAttachementRefs[0],
		nullptr,
		nullptr,
		0, nullptr,
	};

	const vk::SubpassDescription subpass1 {
        {},
        vk::PipelineBindPoint::eGraphics,
        0,nullptr,
        1, &this->colorAttachementRefs[0],
        nullptr,
        nullptr,
        0, nullptr,
	};

	this->subpasses.emplace_back(subpass0);
	//this->subpasses.emplace_back(subpass1);

	const vk::SubpassDependency dependency_imageAcquire {
		VK_SUBPASS_EXTERNAL,
		0,
		vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::AccessFlagBits::eMemoryRead,
		vk::AccessFlagBits::eColorAttachmentWrite,
		{}
	};

//	const vk::SubpassDependency dependency_subpass0Finished {
//		0,
//		1,
//		vk::PipelineStageFlagBits::eColorAttachmentOutput,
//		vk::PipelineStageFlagBits::eColorAttachmentOutput,
//		{},
//		vk::AccessFlagBits::eColorAttachmentWrite,
//		{}
//	};

	this->dependencies = { dependency_imageAcquire };//, dependency_subpass0Finished };

	this->rpci.attachmentCount = static_cast<std::uint32_t>(this->renderPassAttachements.size());
	this->rpci.pAttachments = this->renderPassAttachements.data();

	this->rpci.subpassCount = static_cast<std::uint32_t>(this->subpasses.size());
	this->rpci.pSubpasses = this->subpasses.data();

	this->rpci.dependencyCount = static_cast<std::uint32_t>(this->dependencies.size());
	this->rpci.pDependencies = this->dependencies.data();

	createPassAndFrameBuffers();
}

void renderpass::updateFormat() noexcept {
	this->colorAttachements[0].format = this->swapChain.format.format;
}

void renderpass::createPassAndFrameBuffers() {
	this->renderPass = this->engine.logicalDevice->createRenderPassUnique(rpci);

	this->frameBuffers.resize(this->swapChain.swapChainImages.size());

	for (std::size_t i = 0; i < this->swapChain.swapChainImages.size(); ++i) {
		std::vector fbAttachments{ this->swapChain.swapChainImageViews[i].get() };

		vk::FramebufferCreateInfo fbci {
				{},
				this->renderPass.get(),
				static_cast<std::uint32_t>(fbAttachments.size()),
				fbAttachments.data(),
				this->swapChain.extent.width,
				this->swapChain.extent.height,
				1
		};

		this->frameBuffers[i] = this->engine.logicalDevice->createFramebufferUnique(fbci);
	}
}

void renderpass::begin(const vk::UniqueCommandBuffer& buffer, std::size_t index, const vk::Rect2D &renderArea, const std::vector<vk::ClearValue> &clearValues, vk::SubpassContents contents) {
	vk::RenderPassBeginInfo rpbi {
		this->renderPass.get(),
		this->frameBuffers[index].get(),
		renderArea,
		static_cast<std::uint32_t>(clearValues.size()), clearValues.data()
	};

	buffer->beginRenderPass(rpbi, contents);
}

void renderpass::inherit(vk::CommandBufferInheritanceInfo& cbii, bool includeFramebuffer) {
	cbii.renderPass = this->renderPass.get();
	cbii.subpass = 0;
}
