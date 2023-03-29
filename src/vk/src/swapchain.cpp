#include "swapchain.h"

#include <spdlog/spdlog.h>
#include <isdebug.h>
#include "vk_helper.h"

swapchain::swapchain(const engine_vk& engine) : engine(engine) {
	const auto& physicalDevice = this->engine.physicalDevice;
	const auto& surface = this->engine.surface;

	const auto chooseSurfaceFormat = [&physicalDevice, &surface]() -> vk::SurfaceFormatKHR {
		const auto availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);

		if constexpr (com::isDebug) {
			spdlog::get("graphics")->debug("Formats:");
			for (const auto& availableFormat : availableFormats) {
				spdlog::get("graphics")->debug("\t{} {}", availableFormat.format, availableFormat.colorSpace);
			}
		}

		if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined) {
			return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
		}

		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == vk::Format::eB8G8R8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	};

	this->format = chooseSurfaceFormat();

	if constexpr (com::isDebug) {
		spdlog::get("graphics")->debug("Selected: {} {}", this->format.format, this->format.colorSpace);
	}

	const auto choosePresentMode = [&physicalDevice, &surface]() {
		const auto availablePresentModes  = physicalDevice.getSurfacePresentModesKHR(*surface);

		auto fallbackMode = vk::PresentModeKHR::eFifo;

		if constexpr (com::isDebug) {
			spdlog::get("graphics")->debug("Present Modes:");
			for (const auto& mode : availablePresentModes) {
				spdlog::get("graphics")->debug("\t{}", mode);
			}
		}

		for (const auto& mode : availablePresentModes) {
			if (mode == vk::PresentModeKHR::eMailbox) {
                return mode;
            } else if (mode == vk::PresentModeKHR::eImmediate) {
                fallbackMode = mode;
            }
		}

		return fallbackMode;
	};

	this->presentMode = choosePresentMode();

	if constexpr (com::isDebug) {
		spdlog::get("graphics")->debug("Selected: {}", this->presentMode);
	}

	const auto capabilities = this->engine.physicalDevice.getSurfaceCapabilitiesKHR(*surface);

	auto imageCount = static_cast<std::uint32_t>(swapchain::SWAPCHAIN_IMAGES);

	if (capabilities.maxImageCount > 0) {
		imageCount = std::clamp(imageCount, capabilities.minImageCount, capabilities.maxImageCount);
	}

	auto sharingMode = vk::SharingMode::eExclusive;
	std::uint32_t indexCount = 0;

	this->queueFamilyIndices[0] = this->engine.graphicsFamilyIndex;
	this->queueFamilyIndices[1] = this->engine.transferFamilyIndex;

	const bool separateQueues = this->engine.separateQueues();

	if (separateQueues) {
		sharingMode = vk::SharingMode::eConcurrent;
		indexCount = 2;
	}

	this->swci.minImageCount = imageCount;
	this->swci.imageArrayLayers = 1;
	this->swci.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	this->swci.imageSharingMode = sharingMode;
	this->swci.queueFamilyIndexCount = indexCount;
	this->swci.pQueueFamilyIndices = this->queueFamilyIndices.data();
	this->swci.preTransform = capabilities.currentTransform;
	this->swci.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	this->swci.presentMode = this->presentMode;
	this->swci.clipped = VK_TRUE;

	createSwapChain();
}

void swapchain::createSwapChain() {
	const auto& physicalDevice = this->engine.physicalDevice;
	const auto& surface = this->engine.surface;

	const auto chooseExtent = [window = this->engine.window, &physicalDevice, &surface]() {
		const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);

		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			vk::Extent2D actualExtent {
					static_cast<std::uint32_t>(width),
					static_cast<std::uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	};

	this->extent = chooseExtent();

	this->swci.imageExtent = this->extent;

	this->swci.imageFormat = this->format.format;
	this->swci.imageColorSpace = this->format.colorSpace;

	this->swci.surface = *surface;

	this->swapChain.reset(); //Can have only one SwapchainKHR at a time

	this->swapChain = this->engine.logicalDevice->createSwapchainKHRUnique(this->swci);

	this->swapChainImages = this->engine.logicalDevice->getSwapchainImagesKHR(this->swapChain.get());
	this->swapChainImageViews.resize(this->swapChainImages.size());

	vk::ImageSubresourceRange range {
			vk::ImageAspectFlagBits::eColor,
			0,
			1,
			0,
			1
	};

	for(std::size_t i = 0; i < this->swapChainImages.size(); ++i) {
		const auto& image = this->swapChainImages[i];

		vk::ImageViewCreateInfo iwci {
				{},
				image,
				vk::ImageViewType::e2D,
				this->format.format,
				{},
				range
		};

		this->swapChainImageViews[i] = this->engine.logicalDevice->createImageViewUnique(iwci);
	}
}

std::uint32_t swapchain::acquireNextImage(const vk::Semaphore& semaphore) const {
	return this->engine.logicalDevice->acquireNextImageKHR(this->swapChain.get(), std::numeric_limits<uint64_t>::max(), semaphore, nullptr).value;
}

vk::Result swapchain::present(const std::vector<vk::Semaphore>& waitSemaphores, std::uint32_t index) const {
	vk::PresentInfoKHR pi {
		static_cast<std::uint32_t>(waitSemaphores.size()), waitSemaphores.data(),
		1, &this->swapChain.get(),
		&index, nullptr
	};
	return this->engine.present(pi);
}
