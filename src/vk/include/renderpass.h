#ifndef DISPLAY_RENDERPASS_H
#define DISPLAY_RENDERPASS_H

#include "swapchain.h"

class renderpass {
	friend class pipeline;
private:
	const engine_vk& engine;
	const swapchain& swapChain;

	std::vector<vk::AttachmentDescription> colorAttachements{};
	std::vector<vk::AttachmentReference> colorAttachementRefs{};
	std::vector<vk::AttachmentDescription> renderPassAttachements{};
	std::vector<vk::SubpassDescription> subpasses{};
	std::vector<vk::SubpassDependency> dependencies{};
	vk::RenderPassCreateInfo rpci{};
	vk::UniqueRenderPass renderPass;

	std::vector<vk::UniqueFramebuffer> frameBuffers{};

public:
	renderpass(const engine_vk& engine, const class swapchain& swapchain);
	void updateFormat() noexcept;
	void createPassAndFrameBuffers();

	void begin(const vk::UniqueCommandBuffer& buffer, std::size_t index, const vk::Rect2D& renderArea, const std::vector<vk::ClearValue>& clearValues, vk::SubpassContents contents);
	void inherit(vk::CommandBufferInheritanceInfo& cbii, bool includeFramebuffer = true);

};

#endif //DISPLAY_RENDERPASS_H
