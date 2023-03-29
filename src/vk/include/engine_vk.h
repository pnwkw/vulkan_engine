#ifndef DISPLAY_ENGINE_VK_H
#define DISPLAY_ENGINE_VK_H

#include <vulkan/vulkan.hpp>
#include <functional>
#include <optional>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

using UniqueSurfaceKHR = std::unique_ptr<vk::SurfaceKHR, std::function<void (vk::SurfaceKHR*)>>;

class engine_vk {
	friend class swapchain;
	friend class pipeline;
	friend class renderpass;
	friend class gui;
public:
	class vk_buffer {
	public:
		vk::UniqueDeviceMemory memory;
		vk::UniqueBuffer buffer;
	public:
		vk_buffer() = default;
		vk_buffer(vk::UniqueDeviceMemory& memory, vk::UniqueBuffer& buffer) : memory(std::move(memory)), buffer(std::move(buffer)) {};
	};

	class memory_mapping {
	private:
		const engine_vk& engine;
		const vk::DeviceMemory& memory;
		void* data;
	public:
		memory_mapping(const engine_vk& engine, const vk::DeviceMemory& memory, vk::DeviceSize offset, vk::DeviceSize size): engine(engine), memory(memory) {
			this->data = this->engine.logicalDevice->mapMemory(this->memory, offset, size);
		};
		~memory_mapping() { this->engine.logicalDevice->unmapMemory(memory); };
		[[nodiscard]] void* get() const noexcept { return this->data; };
	};

private:
	GLFWwindow* window;

	vk::UniqueInstance instance;
	vk::DispatchLoaderDynamic dldid;
	using DebugReportCallbackEXTDynamic = vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic>;
	DebugReportCallbackEXTDynamic callback;

	UniqueSurfaceKHR surface;

	vk::PhysicalDevice physicalDevice;
	vk::UniqueDevice logicalDevice;

	std::uint32_t graphicsFamilyIndex;
	vk::Queue graphicsQueue;
	vk::UniqueCommandPool graphicsPool;

	std::uint32_t transferFamilyIndex;
	vk::Queue transferQueue;
	vk::UniqueCommandPool transferPool;

public:
	explicit engine_vk(GLFWwindow* window);

	[[nodiscard]] vk::UniqueShaderModule createShaderModule(const std::string &filename) const;
	[[nodiscard]] vk::UniqueSemaphore createSemaphore() const;
	[[nodiscard]] vk::UniqueFence createFence(vk::FenceCreateFlagBits flags = {}) const;
	[[nodiscard]] std::vector<vk::UniqueCommandBuffer> allocateCmdBuffers(const vk::QueueFlagBits& family, const vk::CommandBufferLevel& level, std::size_t count) const;
	[[nodiscard]] vk_buffer createBuffer(vk::DeviceSize size, const vk::BufferUsageFlags& usage, const vk::MemoryPropertyFlags& properties) const;
	[[nodiscard]] vk_buffer createLocalBufferWithData(vk::DeviceSize size, vk::BufferUsageFlagBits usage, const void *dataPointer) const;
	void updateDescriptorSets(const vk::WriteDescriptorSet& wds) const noexcept;

	[[nodiscard]] vk::Result submit(const vk::QueueFlagBits& family, const vk::SubmitInfo& si, const vk::Fence& fence) const noexcept;
	void waitFence(const vk::Fence& fence) const noexcept;
	void resetFence(const vk::Fence& fence) const noexcept;
	void waitQueueIdle(const vk::QueueFlagBits& family) const noexcept;
	void waitDeviceIdle() const noexcept;

	void copy(vk_buffer& bufferDst, const void* bufferSrc, vk::DeviceSize offsetDst, vk::DeviceSize offsetSrc, vk::DeviceSize size) const;
	void copy(void* bufferDst, const vk_buffer& bufferSrc, vk::DeviceSize offsetDst, vk::DeviceSize offsetSrc, vk::DeviceSize size) const;
	void copy(vk_buffer& bufferDst, const vk_buffer& bufferSrc, vk::DeviceSize offsetDst, vk::DeviceSize offsetSrc, vk::DeviceSize size) const;

private:
	[[nodiscard]] inline bool separateQueues() const noexcept { return this->transferFamilyIndex != this->graphicsFamilyIndex; };
	[[nodiscard]] std::optional<std::uint32_t> findMemoryType(std::uint32_t typeFilter, const vk::MemoryPropertyFlags& properties) const noexcept;
	void createInstance(vk::ApplicationInfo& ai) noexcept;
	void selectPhysicalDevice() noexcept;
	void createLogicalDevice() noexcept;
	[[nodiscard]] vk::Result present(const vk::PresentInfoKHR& pi) const;
	void executeOnQueue(const vk::QueueFlagBits& family, const std::function<void(const vk::CommandBuffer&)>& lambda) const;

};

#endif //DISPLAY_ENGINE_VK_H
