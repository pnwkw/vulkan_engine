#ifndef DISPLAY_SWAPCHAIN_H
#define DISPLAY_SWAPCHAIN_H

#include "engine_vk.h"

class swapchain {
	friend class renderpass;
public:
	constexpr static std::size_t SWAPCHAIN_IMAGES = 3;
	constexpr static std::size_t FRAMES_IN_FLIGHT = 4;
private:
	const engine_vk& engine;

	std::array<std::uint32_t, 2> queueFamilyIndices;

	vk::SurfaceFormatKHR format;
	vk::PresentModeKHR presentMode;
	vk::Extent2D extent;

	vk::SwapchainCreateInfoKHR swci{};
	vk::UniqueSwapchainKHR swapChain;
	std::vector<vk::Image> swapChainImages;
	std::vector<vk::UniqueImageView> swapChainImageViews;

public:
	explicit swapchain(const engine_vk& engine);
	void createSwapChain();
	[[nodiscard]] const vk::Extent2D& getExtent() const noexcept { return this->extent; };
	[[nodiscard]] std::size_t getNumImages() const noexcept { return this->swapChainImages.size(); };
	[[nodiscard]] std::uint32_t acquireNextImage(const vk::Semaphore& semaphore) const;
	vk::Result present(const std::vector<vk::Semaphore>& waitSemaphores, std::uint32_t index) const;
};


#endif //DISPLAY_SWAPCHAIN_H
