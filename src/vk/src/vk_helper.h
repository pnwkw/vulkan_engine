#ifndef DISPLAY_VK_HELPER_H
#define DISPLAY_VK_HELPER_H

#include <cstdint>
#include <utility>
#include <type_traits>
#include <vector>

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>

namespace vk_helper {
	inline bool layersSupported(const std::vector<const char*>& requiredLayers, const std::vector<vk::LayerProperties>& availableLayers) {
		return std::all_of(
				std::begin(requiredLayers), std::end(requiredLayers),
				[&availableLayers](const auto& reqLayer){
					return std::end(availableLayers) !=
						   std::find_if(std::begin(availableLayers), std::end(availableLayers),
										[&reqLayer](const auto& curLayer) {
											return std::strcmp(reqLayer, curLayer.layerName) == 0;
										});
				}
		);
	}

	inline bool extensionsSupported(const std::vector<const char*>& requiredExtensions, const std::vector<vk::ExtensionProperties>& availableExtensions) {
		return std::all_of(
				std::begin(requiredExtensions), std::end(requiredExtensions),
				[&availableExtensions](const auto& reqExtension){
					return std::end(availableExtensions) !=
						   std::find_if(std::begin(availableExtensions), std::end(availableExtensions),
										[&reqExtension](const auto& curExtension) {
											return std::strcmp(reqExtension, curExtension.extensionName) == 0;
										});
				}
		);
	}

    template <typename T>
    using is_vk_to_string_callable = std::enable_if_t<std::is_same_v<decltype(vk::to_string(std::declval<T>())), std::string>>;
}

template<typename T, typename Char>
struct fmt::formatter<T, Char, vk_helper::is_vk_to_string_callable<T>> : fmt::formatter<std::string, Char>
{
    template <typename FormatContext>
    auto format(const T &obj, FormatContext &ctx) {
        return fmt::formatter<std::string>::format(vk::to_string(obj), ctx);
    }
};

#endif //DISPLAY_VK_HELPER_H
