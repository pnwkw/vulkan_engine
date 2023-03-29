#include "engine_vk.h"

#include <array>
#include <app_com.h>
#include <bitset>
#include <isdebug.h>
#include <optional>
#include <spdlog/spdlog.h>
#include "vk_helper.h"

VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
								  VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
								  const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
								  void*                                            pUserData) {
	auto severity = static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity);

	if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
		//spdlog::get("graphics")->info("{}", pCallbackData->pMessage);
	} else if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
		spdlog::get("graphics")->error("{}:{}:{}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
	} else if(severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
		spdlog::get("graphics")->warn("{}:{}:{}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
	} else if (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose) {
		//spdlog::get("graphics")->info("{}", pCallbackData->pMessage);
	}

	return VK_FALSE;
}

auto getDebugMessengerCreateConfig() {
	return vk::DebugUtilsMessengerCreateInfoEXT{
		{},
		vk::FlagTraits<vk::DebugUtilsMessageSeverityFlagBitsEXT>::allFlags,
		vk::FlagTraits<vk::DebugUtilsMessageTypeFlagBitsEXT>::allFlags,
		debugCallback
	};
}

engine_vk::engine_vk(GLFWwindow* window) : window(window) {
	vk::ApplicationInfo ai{
		"Vulkan",
		VK_MAKE_VERSION(1, 0, 0),
		"No Engine",
		VK_MAKE_VERSION(1, 0, 0),
		VK_API_VERSION_1_2
	};

	createInstance(ai);

	this->surface = UniqueSurfaceKHR(new vk::SurfaceKHR, [&instance = this->instance.get()](vk::SurfaceKHR* s) {
		instance.destroySurfaceKHR(*s);
	});

	auto res = static_cast<vk::Result>(glfwCreateWindowSurface(static_cast<VkInstance>(this->instance.get()), this->window, nullptr, reinterpret_cast<VkSurfaceKHR*>(this->surface.get())));

	if (res != vk::Result::eSuccess) {
		spdlog::get("glfw")->error("Can't create vk::SurfaceKHR for presentation!");
		exit(EXIT_FAILURE);
	}

	selectPhysicalDevice();
	createLogicalDevice();
}

void engine_vk::createInstance(vk::ApplicationInfo& ai) noexcept {
	const std::string debugLayer = "VK_LAYER_KHRONOS_validation";

	const auto getRequiredValidationLayers = [&debugLayer]() {
		std::vector<const char*> layers;

		if constexpr (com::isDebug) {
			layers.emplace_back(debugLayer.c_str());
		}

		return layers;
	};

	const auto requiredLayers = getRequiredValidationLayers();
	const auto availableLayers = vk::enumerateInstanceLayerProperties();

	if constexpr (com::isDebug) {
		spdlog::get("graphics")->debug("Required Instance Layers:");
		for(const auto& layer : requiredLayers) {
			spdlog::get("graphics")->debug("\t{}", layer);
		}

		spdlog::get("graphics")->debug("Available Instance Layers:");
		for (const auto& layer : availableLayers) {
			spdlog::get("graphics")->debug("\t{}", layer.layerName);
		}
	}

	auto layersSupported = vk_helper::layersSupported(requiredLayers, availableLayers);

	if(!layersSupported) {
		spdlog::get("graphics")->error("Not all required Instance Layers supported!");
		exit(EXIT_FAILURE);
	}

	const std::string debugExtension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	const auto getRequiredInstanceExtensions = [&debugExtension]() {
		std::uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if constexpr (com::isDebug) {
			extensions.emplace_back(debugExtension.c_str());
		}

		return extensions;
	};

	const auto requiredExtensions = getRequiredInstanceExtensions();
	const auto availableExtensions = vk::enumerateInstanceExtensionProperties();

	if constexpr (com::isDebug) {
		spdlog::get("graphics")->debug("Required Instance Extensions:");
		for(const auto& extension : requiredExtensions) {
			spdlog::get("graphics")->debug("\t{}", extension);
		}

		spdlog::get("graphics")->debug("Available Instance Extensions:");
		for (const auto& extension : availableExtensions) {
			spdlog::get("graphics")->debug("\t{}", extension.extensionName);
		}
	}

	bool extensionsSupported = vk_helper::extensionsSupported(requiredExtensions, availableExtensions);

	if(!extensionsSupported) {
		spdlog::get("graphics")->error("Not all required Instance Extensions supported!");
		exit(EXIT_FAILURE);
	}

	vk::InstanceCreateInfo ici{
		{},
		&ai,
		static_cast<std::uint32_t>(requiredLayers.size()), requiredLayers.data(),
		static_cast<std::uint32_t>(requiredExtensions.size()), requiredExtensions.data()
	};

	if constexpr (com::isDebug) {
		auto dbgci = getDebugMessengerCreateConfig();
		vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> chain(ici, dbgci);
		this->instance = vk::createInstanceUnique(chain.get<vk::InstanceCreateInfo>());
	} else {
		this->instance = vk::createInstanceUnique(ici);
	}

	this->dldid.init(static_cast<VkInstance>(this->instance.get()), vkGetInstanceProcAddr);

	if constexpr (com::isDebug) {
		auto dbgci = getDebugMessengerCreateConfig();

		this->callback = this->instance->createDebugUtilsMessengerEXTUnique(dbgci, nullptr, this->dldid);
	}
}

typedef struct QueueFamilyIndices {
	std::optional<std::uint32_t> graphicsFamily;
	std::optional<std::uint32_t> transferFamily;

	[[nodiscard]] bool isComplete() const noexcept {
		return graphicsFamily.has_value() && transferFamily.has_value();
	}
} QueueFamilyIndices;

const std::string swapChainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
const std::string shaderDrawParameters = VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME;

auto getRequiredDeviceExtensions() {
	std::vector<const char*> extensions;

	extensions.emplace_back(swapChainExtension.c_str());
	//extensions.emplace_back(shaderDrawParameters.c_str());

	return extensions;
};

void engine_vk::selectPhysicalDevice() noexcept {
	const auto availableDevices = this->instance->enumeratePhysicalDevices();

	const auto findQueueFamilies = [surface = this->surface.get()](const vk::PhysicalDevice &d) {
		const auto availableFamilies = d.getQueueFamilyProperties();

		QueueFamilyIndices indices;

		if constexpr (com::isDebug) {
			spdlog::get("graphics")->debug("\tFamilies:");

			for(std::size_t i = 0; i < availableFamilies.size(); ++i) {
				const auto &family = availableFamilies[i];
				spdlog::get("graphics")->debug("\t\t{} [{}]: {}", i, family.queueCount, family.queueFlags);
			}
		}

		for(std::size_t i = 0; i < availableFamilies.size(); ++i) {
			const auto& family = availableFamilies[i];

			bool supportsGraphics = static_cast<bool>(family.queueFlags & vk::QueueFlagBits::eGraphics);
			bool supportsTransfer = static_cast<bool>(family.queueFlags & vk::QueueFlagBits::eTransfer);
			bool supportsPresent = d.getSurfaceSupportKHR(i, *surface);

			const auto index = static_cast<std::uint32_t>(i);

			if (supportsGraphics && supportsPresent) {
				indices.graphicsFamily = index;
			}

			if (supportsTransfer) {
				indices.transferFamily = index;
			}

			if (indices.isComplete())
				break;
		}

		return indices;
	};

	const auto isDeviceSuitable = [surface = this->surface.get(), &findQueueFamilies](const vk::PhysicalDevice &d) {
		const auto& properties = d.getProperties();
		const auto& features = d.getFeatures();

		const auto requiredExtensions = getRequiredDeviceExtensions();
		const auto availableExtensions = d.enumerateDeviceExtensionProperties();

		bool extensionsSupported = vk_helper::extensionsSupported(requiredExtensions, availableExtensions);
		bool swapChainSupported = false;

		if (extensionsSupported) {
			const auto formats = d.getSurfaceFormatsKHR(*surface);
			const auto presentModes = d.getSurfacePresentModesKHR(*surface);
			swapChainSupported = !formats.empty() && !presentModes.empty();
		}

		return
		extensionsSupported && swapChainSupported &&
		properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu &&
		features.geometryShader && features.vertexPipelineStoresAndAtomics;
	};

	if constexpr (com::isDebug)
		spdlog::get("graphics")->debug("Physical Devices:");

	for(const auto& device : availableDevices) {

		if constexpr (com::isDebug) {
			const auto& properties = device.getProperties();
			spdlog::get("graphics")->debug("\t{}", properties.deviceName);
		}

		const bool deviceSuitable = isDeviceSuitable(device);
		const auto& indices = findQueueFamilies(device);
		const bool supportsFamilies = indices.isComplete();

		if (deviceSuitable && supportsFamilies) {
			this->physicalDevice = device;
			this->graphicsFamilyIndex = indices.graphicsFamily.value();
			this->transferFamilyIndex = indices.transferFamily.value();
			break;
		}
	}

	if constexpr (com::isDebug)
		spdlog::get("graphics")->debug("Selected: {} (Graphics {} Transfer {})", this->physicalDevice.getProperties().deviceName, this->graphicsFamilyIndex, this->transferFamilyIndex);
}

void engine_vk::createLogicalDevice() noexcept {
	std::vector<vk::DeviceQueueCreateInfo> queueInfos;

	const std::array<float, 2> defaultPriority{ 1.0f, 1.0f };

	const bool separateQueues = (this->graphicsFamilyIndex != this->transferFamilyIndex);

	const auto availableFamilies = this->physicalDevice.getQueueFamilyProperties();
	const auto graphicsQueueSize = availableFamilies[this->graphicsFamilyIndex].queueCount;

	if (separateQueues) {
		const vk::DeviceQueueCreateInfo dqci_graphics {
			{},
			this->graphicsFamilyIndex,
			1,&defaultPriority[0]
		};

		const vk::DeviceQueueCreateInfo dqci_transfer {
			{},
			this->graphicsFamilyIndex,
			1,&defaultPriority[1]
		};
		queueInfos.emplace_back(dqci_graphics);
		queueInfos.emplace_back(dqci_transfer);
	} else if (graphicsQueueSize == 1) {
		const vk::DeviceQueueCreateInfo dqci_combo {
				{},
				this->graphicsFamilyIndex,
				1, &defaultPriority[0]
		};
		queueInfos.emplace_back(dqci_combo);
	} else {
		const vk::DeviceQueueCreateInfo dqci_combo {
				{},
				this->graphicsFamilyIndex,
				std::size(defaultPriority), defaultPriority.data()
		};
		queueInfos.emplace_back(dqci_combo);
	}

	const auto requiredExtensions = getRequiredDeviceExtensions();

	vk::PhysicalDeviceFeatures pdf {};

    // TODO: customizable pdf request ?
	pdf.geometryShader = VK_TRUE;
	pdf.vertexPipelineStoresAndAtomics = VK_TRUE;

	const vk::DeviceCreateInfo dci {
		{},
		static_cast<std::uint32_t>(queueInfos.size()), queueInfos.data(),
		0,nullptr,
		static_cast<std::uint32_t>(requiredExtensions.size()), requiredExtensions.data(),
		&pdf
	};

	this->logicalDevice = this->physicalDevice.createDeviceUnique(dci);

	if (separateQueues || graphicsQueueSize == 1) {
		this->graphicsQueue = this->logicalDevice->getQueue(this->graphicsFamilyIndex, 0);
		this->transferQueue = this->logicalDevice->getQueue(this->transferFamilyIndex, 0);
	} else {
		this->graphicsQueue = this->logicalDevice->getQueue(this->graphicsFamilyIndex, 0);
		this->transferQueue = this->logicalDevice->getQueue(this->transferFamilyIndex, 1);
	}

	const vk::CommandPoolCreateInfo cpci_graphics {
        vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		this->graphicsFamilyIndex
	};

	const vk::CommandPoolCreateInfo cpci_transfer {
		{},
		this->transferFamilyIndex
	};

	this->graphicsPool = this->logicalDevice->createCommandPoolUnique(cpci_graphics);
	this->transferPool = this->logicalDevice->createCommandPoolUnique(cpci_transfer);
}

vk::UniqueShaderModule engine_vk::createShaderModule(const std::string &filename) const {
	const auto buffer = app_com::loadShader(filename);

	const vk::ShaderModuleCreateInfo smci {
		{},
		static_cast<std::uint32_t>(buffer.size()), reinterpret_cast<const std::uint32_t*>(buffer.data())
	};

	return this->logicalDevice->createShaderModuleUnique(smci);
}

vk::UniqueSemaphore engine_vk::createSemaphore() const {
	const vk::SemaphoreCreateInfo sci {};
	return this->logicalDevice->createSemaphoreUnique(sci);
}

vk::UniqueFence engine_vk::createFence(vk::FenceCreateFlagBits flags) const {
	const vk::FenceCreateInfo fci {
		flags
	};
	return this->logicalDevice->createFenceUnique(fci);
}

std::optional<std::uint32_t> engine_vk::findMemoryType(std::uint32_t typeFilter, const vk::MemoryPropertyFlags& properties) const noexcept {
	const vk::PhysicalDeviceMemoryProperties memProperties = this->physicalDevice.getMemoryProperties();

	const auto& bits = std::bitset<32>(typeFilter);
	for (std::uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		const auto& memoryType = memProperties.memoryTypes[i];

		if constexpr (com::isDebug) {
			spdlog::get("graphics")->debug("[{}] Mem type: {}", i, memoryType.propertyFlags);
		}

		if (bits[i] && (memoryType.propertyFlags & properties)) {
			return i;
		}
	}
	return {};
}

std::vector<vk::UniqueCommandBuffer> engine_vk::allocateCmdBuffers(const vk::QueueFlagBits& family, const vk::CommandBufferLevel& level, std::size_t count) const {
	vk::CommandPool cmdPool;

	switch(family) {
		case vk::QueueFlagBits::eGraphics:
			cmdPool = this->graphicsPool.get();
			break;
		case vk::QueueFlagBits::eTransfer:
			cmdPool = this->transferPool.get();
			break;
		default:
			break;
	}

	const vk::CommandBufferAllocateInfo cbai {
			cmdPool,
			level,
			static_cast<std::uint32_t>(count)
	};

	return this->logicalDevice->allocateCommandBuffersUnique(cbai);
}

engine_vk::vk_buffer engine_vk::createBuffer(vk::DeviceSize size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& properties) const {
	const vk::BufferCreateInfo bci {
		{},
		size,
		usage,
		vk::SharingMode::eExclusive
	};

	auto buffer = this->logicalDevice->createBufferUnique(bci);

	const auto& memRequirements = this->logicalDevice->getBufferMemoryRequirements(buffer.get());

	const vk::MemoryAllocateInfo mai {
		memRequirements.size,
		findMemoryType(memRequirements.memoryTypeBits, properties).value()
	};

	auto bufferMemory = this->logicalDevice->allocateMemoryUnique(mai);

	this->logicalDevice->bindBufferMemory(buffer.get(), bufferMemory.get(), 0);

	return engine_vk::vk_buffer(bufferMemory, buffer);
}

vk::Result engine_vk::submit(const vk::QueueFlagBits& family, const vk::SubmitInfo &si, const vk::Fence& fence) const noexcept {
	switch(family) {
		case vk::QueueFlagBits::eGraphics:
			return this->graphicsQueue.submit(1, &si, fence);
		case vk::QueueFlagBits::eTransfer:
			return this->transferQueue.submit(1, &si, fence);
		default:
			return vk::Result::eErrorUnknown;
	}
}

vk::Result engine_vk::present(const vk::PresentInfoKHR& pi) const {
	return this->graphicsQueue.presentKHR(pi);
}

void engine_vk::waitFence(const vk::Fence &fence) const noexcept {
	this->logicalDevice->waitForFences(1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
}

void engine_vk::resetFence(const vk::Fence &fence) const noexcept {
	this->logicalDevice->resetFences(1, &fence);
}

void engine_vk::waitQueueIdle(const vk::QueueFlagBits& family) const noexcept {
	switch(family) {
		case vk::QueueFlagBits::eGraphics:
			return this->graphicsQueue.waitIdle();
		case vk::QueueFlagBits::eTransfer:
			return this->transferQueue.waitIdle();
		default:
			return;
	}
}

void engine_vk::waitDeviceIdle() const noexcept {
	this->logicalDevice->waitIdle();
}

void engine_vk::executeOnQueue(const vk::QueueFlagBits& family, const std::function<void(const vk::CommandBuffer&)>& lambda) const {
	vk::UniqueCommandBuffer commandBuffer = std::move(allocateCmdBuffers(family, vk::CommandBufferLevel::ePrimary, 1)[0]);

	vk::CommandBufferBeginInfo cbbi {};
	cbbi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	commandBuffer->begin(cbbi);

	lambda(*commandBuffer);

	commandBuffer->end();

	vk::SubmitInfo si {};
	si.commandBufferCount = 1;
	si.pCommandBuffers = &commandBuffer.get();

	static_cast<void>(submit(family, si, {}));
	waitQueueIdle(family);
}

void engine_vk::copy(engine_vk::vk_buffer& bufferDst, const void* bufferSrc, vk::DeviceSize offsetDst, vk::DeviceSize offsetSrc, vk::DeviceSize size) const {
	memory_mapping map(*this, bufferDst.memory.get(), offsetDst, size);
	const auto bufferSrcOffset = static_cast<const unsigned char*>(bufferSrc) + offsetSrc;
	std::memcpy(map.get(), bufferSrcOffset, size);
}

void engine_vk::copy(void *bufferDst, const engine_vk::vk_buffer &bufferSrc, vk::DeviceSize offsetDst, vk::DeviceSize offsetSrc, vk::DeviceSize size) const {
	memory_mapping map(*this, bufferSrc.memory.get(), offsetSrc, size);
	const auto bufferDstOffset = static_cast<unsigned char*>(bufferDst) + offsetDst;
	std::memcpy(bufferDstOffset, map.get(), size);
}

void engine_vk::copy(vk_buffer& bufferDst, const vk_buffer& bufferSrc, vk::DeviceSize offsetDst, vk::DeviceSize offsetSrc, vk::DeviceSize size) const {
	vk::BufferCopy copyRegion { offsetSrc, offsetDst, size };

	executeOnQueue(vk::QueueFlagBits::eTransfer,
				[&bufferDst, &bufferSrc, &copyRegion](const vk::CommandBuffer& buffer) {
					buffer.copyBuffer(bufferSrc.buffer.get(), bufferDst.buffer.get(), 1, &copyRegion);
				}
	);
}

engine_vk::vk_buffer engine_vk::createLocalBufferWithData(vk::DeviceSize size, vk::BufferUsageFlagBits usage, const void *dataPointer) const {
	auto staging = createBuffer(size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	copy(staging, dataPointer, 0, 0, size);

	auto local = createBuffer(size, usage | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
	copy(local, staging, 0, 0, size);

	return local;
}

void engine_vk::updateDescriptorSets(const vk::WriteDescriptorSet& wds) const noexcept {
	this->logicalDevice->updateDescriptorSets(1, &wds, 0, nullptr);
}
